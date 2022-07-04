/* Copyright 2016-2022 Matt Evans
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sys/mman.h>
#include <string.h>

#include "PPCCPUState.h"
#include "PPCMMU.h"
#include "PPCInterpreter.h"
#include "types.h"
#include "blockstore.h"
#include "op_addrs.h"

#define BS_CODE_SIZE	(4*1024*1024)
#define BS_FUMES	(1024)
#define BS_GRACE	100

static u8 block_buffer_storage[BS_CODE_SIZE + 4096];
static u8 *block_buffer = 0;
static u8 *block_buffer_last = 0;

static block_t dummy_block;
block_t *last_block = &dummy_block;
unsigned int bs_gencount = 0;

static const unsigned int headerlen = ALIGN_TO(sizeof(block_t), 64);


/******************************************************************************/
#define HTAB_ENTRIES	32768
/* TODO: Multiply by a prime, measure efficiency... */
#define HASH_IDX(va, msr)	((((va) >> 2) ^ ((va) >> 13) ^ (msr)) & (HTAB_ENTRIES - 1))

static block_t *bs_hashmap[HTAB_ENTRIES];

static inline void bs_hm_insert(PA pc_pa, u32 msr, block_t *block)
{
	unsigned int i = HASH_IDX(pc_pa, msr);
	block_t *b = bs_hashmap[i];
	if (b) {
		block->next = b; // FIXME: could just write this unconditionally.  ProfileMe.
	}
	bs_hashmap[i] = block;
}

static inline block_t *bs_hm_lookup(PA pc_pa, u32 msr)
{
	unsigned int i = HASH_IDX(pc_pa, msr);
	block_t *b = bs_hashmap[i];

	bool firstTry = true;
	while (b) {
		if (b->pc_pa == pc_pa && b->msr == msr) {
			last_block = b;
			if (firstTry)
				COUNT(CTR_JIT_HASH_HIT_FIRST);
			else
				COUNT(CTR_JIT_HASH_HIT);
			return b;
		}
		firstTry = false;
		b = b->next;
	}
	COUNT(CTR_JIT_HASH_MISS);
	return 0;
}

block_t *findBlock_slow(PPCMMU *mmu, PPCCPUState *pcs, PPCMMU::fault_t *fault)
{
	/* First, make sure PC translates to a valid PA.  If it doesn't, it's
	 * not legit to find the block. (And, if it's been unmapped, block will
	 * have been erased anyway.)
	 */
	VA pc = pcs->getPC();

	/* See if the VA we're looking for is actually accessible.
	 *
	 * Can this be optimised out if we *know* there've been no TLBIs since
	 * last time?  I don't think so, because HTAB is lossy and ISI/DSI could
	 * Just Happen simply because some other lookup knocked something else
	 * out of the HTAB.  Maybe if we know no faults have occurred either..?
	 *
	 * Note bs_gencount tracks MMU mapping changes; that isn't checked here
	 * as we no longer empty the blockstore on mapping changes (yay!).
	 */
	u64 pa;
	if (!mmu->translateAddr(pc, &pa, true, true, pcs->isPrivileged(), fault)) {
		JITTRACE("findBlock: Translating PC %08x failed, fault %d\n", pc, *fault);
		return 0;
	} else {
		*fault = PPCMMU::FAULT_NONE;
		JITTRACE("findBlock: Translated PC %08x to %016lx\n", pc, pa);
	}

	/* Blocks are indexed/matched by PA now: */
	return bs_hm_lookup(pa, pcs->getMSR());
}

static void _resetBlockstore(void)
{
	/* Reset allocator */
	block_buffer_last = block_buffer;
	last_block = &dummy_block;

	/* Reset hashmap */
	memset(bs_hashmap, 0, sizeof(bs_hashmap));
}

void resetBlockstore(void)
{
	_resetBlockstore();
	COUNT(CTR_JIT_RESET_BS);
}

/* Allocates a block_t and finds space in the code buffer to begin generating
 * code to.  Note that our sophisticated "fuck it all" cleanup policy means that
 * the code buffer simply grows and is reset to empty if space is exhausted.
 * Thus this returns the number of bytes free at the time of this allocation,
 * assuming that generation will immediately follow.  Generation then calls
 * amendBlockSize with the amount of real space used.  Don't call this without
 * following with amendBlockSize().
 *
 * A block_t is immediately followed by the generated code.  Keep block_t a multiple of CL!
 */
block_t *allocBlock(VA pc, PA pc_pa, u32 msr, unsigned int *bytes_avail)
{
	// FIXME: Assert the block's not found
retry:
	/* First, check if there's a "reasonable" amount of code space free; if
	 * there isn't, there's no point starting to generate the block and then
	 * failing then.  This number is art not science:
	 */
	if ((block_buffer_last - block_buffer) > (BS_CODE_SIZE - BS_FUMES)) {
		JITTRACE("Block buffer exhausted\n");
		resetBlockstore();
		goto retry;
	}

	/* Allocate space in block_buffer */
	block_t *b = (block_t *)block_buffer_last;

	b->pc = pc;	/* This is stored for findBlock() shortcut purposes */
	b->pc_pa = pc_pa;	/* This is the primary index */
	b->msr = msr;
	b->codelen = 0;
	b->next = 0;
	bs_hm_insert(pc_pa, msr, b);

	*bytes_avail = BS_CODE_SIZE - BS_GRACE - (block_buffer_last - block_buffer) - headerlen;

	return b;
}

void 	amendBlockSize(block_t *b, unsigned int real_size)
{
	block_buffer_last += headerlen + real_size;
	b->codelen = real_size;

	/* CL-align next allocation: */
	block_buffer_last = (u8 *)ALIGN_TO(block_buffer_last, 64);
}

void	initBlockStore(void)
{
	block_buffer = (u8 *)ALIGN_TO(block_buffer_storage, 4096);
	mprotect(block_buffer, BS_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
	JITTRACE("Block buffer is at %p, blockstore starts at %p (%d bytes)\n",
		 block_buffer_storage, block_buffer, BS_CODE_SIZE);

	/* dummy_block is intended to be a never-match */
	dummy_block.pc = ~0;
	dummy_block.msr = ~0;

	/* Ensure blockstore is close enough to fns it needs to call: */
	if (labs((s64)get_op_unk() - (s64)block_buffer) > 0x80000000) {
		FATAL("Block buffer is too far from .txt, e.g. buffer at %p, fn at %p\n",
		      block_buffer, get_op_unk());
	}

	_resetBlockstore();
}
