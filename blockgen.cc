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

#include <stdio.h>
#include "log.h"
#include "blockgen.h"
#include "op_addrs.h"
#include "PPCInstructionFields.h"


/* FIXME: Fugly globals -- pass this round in a generation context struct/object. */
static unsigned int cur_blk_size = 0;
static unsigned int cur_blk_nr_instrs = 0;

/* FIXME: Needs some generator context to shove this crap... */
static bool oom_abort = false;

#define CODE_NEEDS(x, s) do { if ((x) < (s)) {                       \
			JITTRACE("Out of space, codelen %d\n", (x)); \
			oom_abort = true;			     \
			return;					     \
		} } while(0)

#define CODE_FINAL(len, used, code)	do { CODE_NEEDS((len), (used)); \
		(len) -= (used); (code) += (used); } while(0)

#define INST8(code, len, val)	do { *(u8 *)  ((code) + (len)) = (val); (len) += 1; } while(0)
#define INST16(code, len, val)	do { *(u16 *) ((code) + (len)) = (val); (len) += 2; } while(0)
#define INST32(code, len, val)	do { *(u32 *) ((code) + (len)) = (val); (len) += 4; } while(0)
#define INST64(code, len, val)	do { *(u64 *) ((code) + (len)) = (val); (len) += 8; } while(0)

/* This is the "JIT backend" (lol).  It doesn't do any code generation besides
 * planting calls to the interpreter functions.  A basic block is therefore
 * simply a list of call sites, followed by a block terminator (return).
 *
 * Arguments are passed in:
 * 	RDI (this), RSI, RDX, RCX, R8, R9
 * Callee-saved:
 *      RBX, RBP, and R12–R15
 *
 * Blocks are called with a PPCInterpreter* in RDI; preserve this and provide to
 * every called function.
 *
 * s16 types appear to be sign-extended from e.g. %dx upon use (in the callee)
 * So could just move32 for all args!
 */

static int get_rel_addr(void *here, u64 dest)
{
	/* Rel address for a callq instruction -- note relative to the end of the 5-byte instruction. */
	return (intptr_t)dest - (intptr_t)here - 5;
}

static void generate_call(u8 **codeptr, unsigned int *codelen, u64 addr)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);	/* movl	$xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);	/* movl	$xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	INST8 (*codeptr, l, 0xba);	/* movl	$xxxxxxxx, %edx */
	INST32(*codeptr, l, arg2);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, int arg3)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);	/* movl	$xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	INST8 (*codeptr, l, 0xba);	/* movl	$xxxxxxxx, %edx */
	INST32(*codeptr, l, arg2);

	INST8 (*codeptr, l, 0xb9);	/* movl	$xxxxxxxx, %ecx */
	INST32(*codeptr, l, arg3);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int_int_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, int arg3, int arg4)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);	/* movl	$xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	INST8 (*codeptr, l, 0xba);	/* movl	$xxxxxxxx, %edx */
	INST32(*codeptr, l, arg2);

	INST8 (*codeptr, l, 0xb9);	/* movl	$xxxxxxxx, %ecx */
	INST32(*codeptr, l, arg3);

	INST8 (*codeptr, l, 0x41);	/* movl $xxxxxxxx, %r8d */
	INST8 (*codeptr, l, 0xb8);
	INST32(*codeptr, l, arg4);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int_int_int_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, int arg3, int arg4, int arg5)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST8 (*codeptr, l, 0x48);	/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);	/* movl	$xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	INST8 (*codeptr, l, 0xba);	/* movl	$xxxxxxxx, %edx */
	INST32(*codeptr, l, arg2);

	INST8 (*codeptr, l, 0xb9);	/* movl	$xxxxxxxx, %ecx */
	INST32(*codeptr, l, arg3);

	INST8 (*codeptr, l, 0x41);	/* movl $xxxxxxxx, %r8d */
	INST8 (*codeptr, l, 0xb8);
	INST32(*codeptr, l, arg4);

	INST8 (*codeptr, l, 0x41);	/* movl $xxxxxxxx, %r9d */
	INST8 (*codeptr, l, 0xb9);
	INST32(*codeptr, l, arg5);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

/* A 7-arg call looks like:
 *    55      	      		pushq   %rbp
 *    48 89 e5        		movq    %rsp, %rbp
 *    48 83 ec 10     		subq    $16, %rsp
 *    48 c7 04 24 07 00 00 00   movq    $7, (%rsp)
 *    bf 01 00 00 00  		movl    $1, %edi
 *    be 02 00 00 00  		movl    $2, %esi
 *    ba 03 00 00 00  		movl    $3, %edx
 *    b9 04 00 00 00  		movl    $4, %ecx
 *    41 b8 05 00 00 00       	movl    $5, %r8d
 *    41 b9 06 00 00 00       	movl    $6, %r9d
 *    e8 00 00 00 00  		callq   foo
 *    31 c0   			xorl    %eax, %eax
 *    48 83 c4 10     		addq    $16, %rsp
 *    5d      			popq    %rbp
 *    c3      			retq
 *
 * FIXME: It's fairly inoptimal to push/pop rbp around every such call.  OTOH
 * it's also inoptimal to do that at block start/end if nothing ends up calling
 * this long-arg fn.
 */
static void generate_call_int_int_int_int_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
	unsigned int l = 0;
	JITTRACE("  %s\n", __FUNCTION__);

	INST32(*codeptr, l, 0x10ec8348);		/* subq    $16, %rsp */

	INST32(*codeptr, l, 0x2404c748);		/* movq    $xxxxxxxx, (%rsp) */
	INST32(*codeptr, l, arg6);

	INST8 (*codeptr, l, 0x48);		/* movq    %rbx, %rdi */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xdf);

	INST8 (*codeptr, l, 0xbe);		/* movl    $xxxxxxxx, %esi */
	INST32(*codeptr, l, arg1);

	INST8 (*codeptr, l, 0xba);		/* movl    $xxxxxxxx, %edx */
	INST32(*codeptr, l, arg2);

	INST8 (*codeptr, l, 0xb9);		/* movl    $xxxxxxxx, %ecx */
	INST32(*codeptr, l, arg3);

	INST8 (*codeptr, l, 0x41);		/* movl    $xxxxxxxx, %r8d */
	INST8 (*codeptr, l, 0xb8);
	INST32(*codeptr, l, arg4);

	INST8 (*codeptr, l, 0x41);		/* movl    $xxxxxxxx, %r8d */
	INST8 (*codeptr, l, 0xb9);
	INST32(*codeptr, l, arg5);

	int r = get_rel_addr(*codeptr + l, addr);
	INST8 (*codeptr, l, 0xe8);	/* callq   $xxxxxxxx */
	INST32(*codeptr, l, r);

	INST32(*codeptr, l, 0x10c48348);		/* addq    $16, %rsp */

	CODE_FINAL(*codelen, l, *codeptr);
}

static void generate_call_int_int_s16(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, s16 arg3)
{
	/* See below */
	generate_call_int_int_int(codeptr, codelen, addr, arg1, arg2, (u16)arg3);
}

static void generate_call_int_int_u16(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, int arg2, u16 arg3)
{
	generate_call_int_int_int(codeptr, codelen, addr, arg1, arg2, arg3);
}

static void generate_call_int_s16_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, s16 arg2, int arg3)
{
	/* My theory is that on x86 s16/u16 just go in a 16-bit reg (e.g. cx,
	 * ax) and the type is only important when extended to a larger width;
	 * so, putting this in the low half of an exx register should be OK.
	 */
	generate_call_int_int_int(codeptr, codelen, addr, arg1, (u16)arg2, arg3);
}

static void generate_call_int_u16_int(u8 **codeptr, unsigned int *codelen, u64 addr, int arg1, u16 arg2, int arg3)
{
	generate_call_int_int_int(codeptr, codelen, addr, arg1, arg2, arg3);
}

static void generate_call_u32_int_int(u8 **codeptr, unsigned int *codelen, u64 addr, u32 arg1, int arg2, int arg3)
{
	generate_call_int_int_int(codeptr, codelen, addr, arg1, arg2, arg3);
}

static void generate_call_u8(u8 **codeptr, unsigned int *codelen, u64 addr, u8 arg1)
{
	generate_call_int(codeptr, codelen, addr, arg1);
}

static void generate_call_u32(u8 **codeptr, unsigned int *codelen, u64 addr, u32 arg1)
{
	generate_call_int(codeptr, codelen, addr, arg1);
}

/* Returns >0 if block-ending instruction emitted:
 * 1: was branch
 * 2: was misc block-ending instr (e.g. RFI)
 *
 * The intention is that type 1 doesn't need to return to runloop and can be
 * used for a simple self-referential block optimisation (or other direct-branch
 * function).
 */
static unsigned int decodeInstrGenerateCall(u32 inst, u8 **codeptr, unsigned int *codelen)
{
	u32 opcode = getOpcode(inst);
	/* Massive switch() that decodes, then calls generate_blah() with address of instr routine. */
#include "AbstractPPCDecoder_generators.h"

	/* Fell through (didn't return); nothing decoded. */
	JITTRACE("Opcode %08x (inst %08x) not decoded: undefined?\n", opcode, inst);
	/* So, call PPCInterpreter::op_unk(u32 inst) to raise the exception. */
	generate_call_u32(codeptr, codelen, get_op_unk(), inst);
	return 2;
}

static void 	plantBlockLoopCheck(VA block_pc, u8 *codestart, u8 **codeptr, unsigned int *codelen)
{
	unsigned int l = 0;
	/* Read PC from PPCCPUState (in PPCInterpreter) and compare to provided PC: */
	INST8 (*codeptr, l, 0x48);	/* movq   offsetof(PPCInterpreter, cpus)(%rbx), %rdi */
	INST8 (*codeptr, l, 0x8b);
	INST8 (*codeptr, l, 0xbb);
	INST32(*codeptr, l, PPCInterpreter::getCPUSoffset());

	INST8 (*codeptr, l, 0x8b);	/* movl   offsetof(PPCCPUState, pc)(%rdi), %edi */
	INST8 (*codeptr, l, 0xbf);
	INST32(*codeptr, l, PPCCPUState::getPCoffset());

	/* Compare PC to block_pc; if the same, branch to codestart */
	INST8 (*codeptr, l, 0x81);	/* cmpl	  $pc, %edi */
	INST8 (*codeptr, l, 0xff);
	INST32(*codeptr, l, block_pc);	/* Note, 32bit PC */

	int r = get_rel_addr(*codeptr + l + 1, (u64)codestart);
	INST8 (*codeptr, l, 0x0f);	/* je	  relative_thing */
	INST8 (*codeptr, l, 0x84);
	INST32(*codeptr, l, r);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void	startBlock(block_t *block, u8 **codeptr, unsigned int *codelen)
{
	unsigned int l = 0;

	INST8 (*codeptr, l, 0x55);	/* pushq   %rbp ; optional, see -fomit-frame-pointer ...*/

	INST8 (*codeptr, l, 0x48);	/* movq    %rsp, %rbp */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xe5);

	INST8 (*codeptr, l, 0x41);	/* pushq   %r12 */
	INST8 (*codeptr, l, 0x54);

	INST8 (*codeptr, l, 0x53);	/* pushq   %rbx */

	/* If this is changed, remember that rsp is 16-byte aligned at every
	 * call instr.  So, the call made it 8B unaligned, push rbp aligned,
	 * push r12 unaligned and push rbx aligns again.  If odd, then include
	 * something like this:
	 * INST32(*codeptr, l, 0x08ec8348);  subq    $8, %rsp
	 */

	/* PPCInterpreter * is stashed in %rbx */
	INST8 (*codeptr, l, 0x48);	/* movq    %rdi, %rbx */
	INST8 (*codeptr, l, 0x89);
	INST8 (*codeptr, l, 0xfb);

	CODE_FINAL(*codelen, l, *codeptr);
}

static void	finaliseBlock(block_t *block, u8 **codeptr, unsigned int *codelen)
{
	unsigned int l = 0;

	INST8 (*codeptr, l, 0xb8);	/* movl    $xxxxxxxx, %eax */
	INST32(*codeptr, l, cur_blk_nr_instrs);	/* FIXME: make dynamic if looping! */

	/* See above re stack alignment.
	 * INST32(*codeptr, l, 0x08c48348);  addq    $8, %rsp
	 */

	INST8 (*codeptr, l, 0x5b);	/* popq	   %rbx */

	INST8 (*codeptr, l, 0x41);	/* popq	   %r12 */
	INST8 (*codeptr, l, 0x5c);

	INST8 (*codeptr, l, 0x5d);	/* popq	   %rbp */

	INST8 (*codeptr, l, 0xc3);	/* retq */

	CODE_FINAL(*codelen, l, *codeptr);

	/* Block is now complete.  Since it's x86, no need to synchronise I&D
	 * caches (haha crikey), but on other backends here's where you'd do
	 * it.
	 */
}

block_t *createBlock(PPCMMU *mmu, PPCCPUState *pcs)
{
retry:
	VA pc = pcs->getPC();
	VA block_start_pc = pc;
	u32 inst;

	PPCMMU::fault_t	fault;

	fault = mmu->loadInst32(pc, &inst, pcs->isPrivileged()); // FIXME: translate & load phys
	if (fault != PPCMMU::FAULT_NONE) {
		// shouldn't get here, outer loop should be checking for faults. just return.
		return 0;
	}

	oom_abort = false;
	unsigned int cur_codelen = 0;

	u64 pc_pa;
	if (!mmu->translateAddr(pc, &pc_pa, true, true, pcs->isPrivileged(), &fault)) {
		JITTRACE("createBlock: Translating PC %08x failed, fault %d\n", pc, fault);
		return 0;
	} else {
		JITTRACE("findBlock: Translated PC %08x to %016lx\n", pc, pc_pa);
	}

	block_t *new_block = allocBlock(pc, pc_pa, pcs->getMSR(), &cur_codelen);

	u8 *cur_codeptr = BLOCK_CODEPTR(new_block);
	u8 *orig_codeptr = cur_codeptr;
	unsigned int orig_codelen = cur_codelen;
	unsigned int must_end_block;

	JITTRACE("Generating block for PC %08x (block %p, code %p)\n", pc, new_block, orig_codeptr);

	cur_blk_nr_instrs = 0;
	cur_blk_size = 0;

	startBlock(new_block, &cur_codeptr, &cur_codelen);

	u8 *start_codeptr = cur_codeptr;

	unsigned int limit = 1000;
	do {
		JITTRACE(" Generating for inst %08x (codeptr %p, len %d)\n",
			 inst, cur_codeptr, cur_codelen);
		must_end_block = decodeInstrGenerateCall(inst, &cur_codeptr, &cur_codelen);
		// if instr == CJ, backwards J or backwards NCB or rfi or sc then end block
		// break
		cur_blk_nr_instrs++;

		if (oom_abort) {
			resetBlockstore();
			goto retry;
		}
		/* Currently, all control-flow instructions break the block. If
		 * ever we run-on through branches etc., this next-PC needs to
		 * get smart/er:
		 */
		pc += 4;
		if ((pc & 0xfff) == 0) {
			/* If crossed page boundary, break block */
			COUNT(CTR_JIT_BLK_SPLIT_PAGE);
			must_end_block = true;
		}

		if (must_end_block) {
			JITTRACE("--- Flagged block end (type %d)\n", must_end_block);
			break;
		}

		/* Read next isntr; if faults, exit to runloop. */
		fault = mmu->loadInst32(pc, &inst, pcs->isPrivileged()); // FIXME: load phys
	} while(--limit != 0 && fault == PPCMMU::FAULT_NONE);

	/* If !must_end_block: e ended the block, but it wasn't on a
	 * block-ending instruction.  So, either a fault occurred or the block
	 * limit was hit.  In either case, that's OK.  PC will be updated by the
	 * instructions that /do/ run, and then next time round the runloop the
	 * fault will be dealt with.
	 */
	if (false && must_end_block == 1) { /* Unused; naive approach is slower */
		/* Ended on a branch:
		 *
	 	 * Plant a check to see if dynamic PC is start of block,
		 * branching inline if so:
		 */
		plantBlockLoopCheck(block_start_pc, start_codeptr, &cur_codeptr, &cur_codelen);
	}

	finaliseBlock(new_block, &cur_codeptr, &cur_codelen);

	// Finally, update blockstore with nr bytes actually consumed:
	amendBlockSize(new_block, orig_codelen - cur_codelen);

	if (JITTRACING) {
		JITTRACE("Block %p (PC %08x MSR %08x) len %d:\n",
			 new_block, new_block->pc, new_block->msr, new_block->codelen);
		int i;
		for (i = 0; i < new_block->codelen; i++) {
			if ((i & 15) == 0)
				JITTRACE("  %03d: ", i);
			lprintf("%02x ", orig_codeptr[i]);
			if ((i & 15) == 15)
				lprintf("\n");
		}
		if ((i & 15) < 15)
			lprintf("\n");
	}

	return new_block;
}
