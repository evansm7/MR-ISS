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

#ifndef BLOCKSTORE_H
#define BLOCKSTORE_H

#include "PPCInterpreter.h"
#include "PPCCPUState.h"
#include "PPCMMU.h"

#define MAX_INSTRS	100
#define MAX_INSTR_SIZE	18
#define CODELEN		(MAX_INSTR_SIZE*MAX_INSTRS)

extern "C" {
	typedef int (*block_fn_t)(PPCInterpreter *);
}

struct _block {
	struct _block *next;
	/* Match state */
	VA pc;
	PA pc_pa;
	u32 msr;
	unsigned int codelen;
};

typedef struct _block block_t;

#define BLOCK_CODEPTR(x)	( ((u8 *)(x)) + ALIGN_TO(sizeof(block_t), 64) )

extern block_t *last_block;
extern unsigned int bs_gencount;

block_t *findBlock_slow(PPCMMU *mmu, PPCCPUState *pcs, PPCMMU::fault_t *fault);
block_t *allocBlock(VA pc, PA pc_pa, u32 msr, unsigned int *bytes_avail);
void 	amendBlockSize(block_t *b, unsigned int real_size);
void	initBlockStore(void);
void 	resetBlockstore(void);

static inline block_t *findBlock(PPCMMU *mmu, PPCCPUState *pcs, PPCMMU::fault_t *fault)
{
	/* This quick shortcut does depend on MMU config staying the same.  If
	 * the gc has changed, don't do this and fall back to the old lookup.
	 */
	if (mmu->getGenCount() == bs_gencount &&
	    last_block->pc == pcs->getPC() &&
	    last_block->msr == pcs->getMSR())
		return last_block;
	else
		return findBlock_slow(mmu, pcs, fault);
}

#endif
