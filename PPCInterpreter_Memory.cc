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

#include "PPCInterpreter.h"

//#define ATOMIX_DEBUG 1

/********************* Memory *********************/

RET_TYPE PPCInterpreter::op_lmw(int RT, int16_t D, int RA0)
{
	REG val_RA0 = READ_GPR_OZ(RA0);

	DISASS("%08x   %08x  lmw\t %d, %d(%d) \n", pc, inst, /* in */ RT, /* in */ D, /* in */ RA0);

	ADR base = val_RA0 + D;

	for (int n = RT; n < 32; n++) {
		u32 v = LOAD32(base);
		MFCHECK(1);
		WRITE_GPR(n, v);
		base += 4;
	}

	incPC();
	COUNT(CTR_INST_LMW);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_lwarx(int RA0, int RB, int EH, int RT)
{
	// Inputs 'RA0,RB,EH', implicit inputs ''
	REG val_RA0 = READ_GPR_OZ(RA0);
	REG val_RB = READ_GPR(RB);
	REG val_RT = 0;

	DISASS("%08x   %08x  lwarx\t %d,  %d, %d (EH %d) \n", pc, inst, /* out */ RT, /* in */ RA0, /* in */ RB, /* in */ EH);
	ADR addr = val_RA0 + val_RB;		/* FIXME, check for alignment */
	val_RT = LOAD32(addr);
	reservation_address = addr;		/* FIXME, should be PA! */
	reservation_gencount = exc_generation_count;
#ifdef ATOMIX_DEBUG
	LOG("lwarx[%p] res_addr %p, gc 0x%lx\n", addr, reservation_address, reservation_gencount);
#endif
	MFCHECK(1);

	WRITE_GPR(RT, val_RT);

	incPC();
	COUNT(CTR_INST_LWARX);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_stmw(int RS, int16_t D, int RA0)
{
	REG val_RA0 = READ_GPR_OZ(RA0);

	DISASS("%08x   %08x  stmw\t %d, %d(%d) \n", pc, inst, /* in */ RS, /* in */ D, /* in */ RA0);

	ADR base = val_RA0 + D;

	for (int n = RS; n < 32; n++) {
		STORE32(base, READ_GPR(n));
		MFCHECK(0);
		base += 4;
	}

	incPC();
	COUNT(CTR_INST_STMW);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_stwcx(int RS, int RA0, int RB)
{
	REG val_RA0 = READ_GPR_OZ(RA0);
	REG val_RB = READ_GPR(RB);
	REG val_RS = READ_GPR(RS);

	DISASS("%08x   %08x  stwcx.\t %d, %d, %d \n", pc, inst, RS, /* in */ RA0, /* in */ RB);

	u32 cr0 = cpus->getXER_SO();
	ADR base = val_RA0 + val_RB;	/* Should be aligned, FIXME check 1:0 */

	if (base != reservation_address ||
	    reservation_gencount != exc_generation_count) {
		/* Failed. */
#ifdef ATOMIX_DEBUG
		LOG("stwcx[%p], cur gc 0x%lx: res_addr %p, gc 0x%lx - FAIL\n",
		    base, exc_generation_count,
		    reservation_address, reservation_gencount);
#endif
		/* FIXME: This should translate/raise exceptions FIRST, then
		 * check reservation.  A stwcx in a RO page will, with this,
		 * never cause the page to fault/become RW?  Also, when we fake
		 * Accessed/Dirty by using DSIs, this needs to raise an access
		 * fault...
		 */
	} else {
#ifdef ATOMIX_DEBUG
		LOG("stwcx[%p], cur gc 0x%lx: res_addr %p, gc 0x%lx - storing %08x OK\n",
		    base, exc_generation_count,
		    reservation_address, reservation_gencount, val_RS);
#endif
		STORE32(base, val_RS);
		MFCHECK(0);
		cr0 |= 2;
	}
	blow_reservation();
	/* ME: What does this note mean? "synch, note bit 0 must be 1 else undef"
	 */
	u32 cr = READ_CR();
	WRITE_CR_FIELD(cr, 0, cr0);
	WRITE_CR(cr);

	incPC();
	COUNT(CTR_INST_STWCX);
	INTERP_RETURN;
}

/* String ops:
 * These are similar to lmw/stmw, except load an arbitrary number of bytes 1-32.
 * Things to count:
 * 1. Unaligned access
 * 2. Nr bytes not divisible by 4
 *
 * Might do these in PALcode...
 */
RET_TYPE PPCInterpreter::op_lswi(int RT, int RA0, int NB)
{
        REG val_RA0 = READ_GPR_OZ(RA0);

        DISASS("%08x   %08x  lswi\t r%d, r%d, #%d \n", pc, inst, /* in */ RT, /* in */ RA0, /* in */ NB);

	/* Load CEIL(NB/4) words from RA into registers starting from RT,
	 * zero-extending final bytes in final register if not a full multiple
	 * of 4 bytes */
	ADR base = val_RA0;
	if (NB == 0)
		NB = 32;

	if (base & 3)
		COUNT(CTR_INST_STRING_LD_UNALIGNED);

	if (NB & 3)
		COUNT(CTR_INST_STRING_LD_ODD);

	int reg = RT;

	if (RT == RA0) {
		/* FIXME: Invalid instruction. Also need to check if RA is in
		 * range of instrs to be loaded.
		 */
	}

	for (int n = 0; n < NB; n += 4) {
		u32 v = LOAD32(base + n);
		MFCHECK(1);
		/* If last word loaded, if there weren't a /4 number of bytes
		 * then we loaded an odd numner and LSBs need masking:
		 */
		if ((NB - n) < 4) {
			u32 mask = (0xffffffff >> (8*(NB-n)));
			v = v & mask;
			DISASS("\tr%d = LD[%08x] = %08x (mask %08x)\n",
			       reg, base + n, v, mask);
		} else {
			DISASS("\tr%d = LD[%08x] = %08x\n",
			       reg, base + n, v);
		}
		WRITE_GPR(reg, v);
		reg = (reg + 1) & 0x1f;
	}

        incPC();
        COUNT(CTR_INST_LSWI);
        INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_lswx(int RT, int RA0, int RB)
{
        REG val_RA0 = READ_GPR_OZ(RA0);
        REG val_RB = READ_GPR(RB);

        DISASS("%08x   %08x  lswx\t r%d, r%d, r%d \n", pc, inst, /* in */ RT, /* in */ RA0, /* in */ RB);

	int numbytes = cpus->getXER() & XER_STR;

	if (RT == RB || RT == RA0) {
		/* Invalid instruction -- FIXME
		 * Also, if RA/RB is in range to be loaded.
		 * This is if RA is >= RT && <= RT+numregs, or
		 * < RT && <= RT+numregs mod 32 ?
		 */
	}

	/* Load words from RA into registers starting from RT,
	 * zero-extending final bytes in final register if not a full multiple
	 * of 4 bytes */
	ADR base = val_RA0 + val_RB;

	if (base & 3)
		COUNT(CTR_INST_STRING_LD_UNALIGNED);

	if (numbytes & 3)
		COUNT(CTR_INST_STRING_LD_ODD);

	int reg = RT;

	for (int n = 0; n < numbytes; n += 4) {
		u32 v = LOAD32(base + n);
		MFCHECK(1);
		/* If last word loaded, if there weren't a /4 number of bytes
		 * then we loaded an odd numner and LSBs need masking:
		 */
		if ((numbytes - n) < 4) {
			u32 mask = (0xffffffff >> (8*(numbytes-n)));
			v = v & mask;
			DISASS("\tr%d = LD[%08x] = %08x (mask %08x)\n",
			       reg, base + n, v, mask);
		} else {
			DISASS("\tr%d = LD[%08x] = %08x\n",
			       reg, base + n, v);
		}
		WRITE_GPR(reg, v);
		reg = (reg + 1) & 0x1f;
	}

        incPC();
        COUNT(CTR_INST_LSWX);
        INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_stswi(int RS, int RA0, int NB)
{
        REG val_RA0 = READ_GPR_OZ(RA0);

        DISASS("%08x   %08x  stswi\t %d, %d, %d \n", pc, inst, /* in */ RS, /* in */ RA0, /* in */ NB);

	ADR base = val_RA0;
	if (NB == 0)
		NB = 32;

	if (base & 3)
		COUNT(CTR_INST_STRING_ST_UNALIGNED);

	if (NB & 3)
		COUNT(CTR_INST_STRING_ST_ODD);

	int reg = RS;

	for (int n = 0; n < NB; n += 4) {
		REG v = READ_GPR(reg);

		/* If < one word to store, store individual bytes */
		if ((NB - n) < 4) {
			for (int i = 0; i < (NB - n); i++) {
				u8 b = (v >> (24 - (i*8))) & 0xff;
				DISASS("\tST[%08x] = r%d(%d) = %02x\n",
				       base + n + i, reg, 3-i, b);
				STORE32(base + n + i, v);
				MFCHECK(0);
			}
		} else {
			DISASS("\tST[%08x] = r%d = %08x\n",
			       base + n, reg, v);
			STORE32(base + n, v);
			MFCHECK(0);
		}
		WRITE_GPR(reg, v);
		reg = (reg + 1) & 0x1f;
	}

        incPC();
        COUNT(CTR_INST_STSWI);
        INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_stswx(int RS, int RA0, int RB)
{
        REG val_RA0 = READ_GPR_OZ(RA0);
        REG val_RB = READ_GPR(RB);

        DISASS("%08x   %08x  stswx\t %d, %d, %d \n", pc, inst, /* in */ RS, /* in */ RA0, /* in */ RB);

	ADR base = val_RA0 + val_RB;
	int numbytes = cpus->getXER() & XER_STR;

	if (base & 3)
		COUNT(CTR_INST_STRING_ST_UNALIGNED);

	if (numbytes & 3)
		COUNT(CTR_INST_STRING_ST_ODD);

	int reg = RS;

	for (int n = 0; n < numbytes; n += 4) {
		REG v = READ_GPR(reg);

		/* If < one word to store, store individual bytes */
		if ((numbytes - n) < 4) {
			for (int i = 0; i < (numbytes - n); i++) {
				u8 b = (v >> (24 - (i*8))) & 0xff;
				DISASS("\tST[%08x] = r%d(%d) = %02x\n",
				       base + n + i, reg, 3-i, b);
				STORE32(base + n + i, v);
				MFCHECK(0);
			}
		} else {
			DISASS("\tST[%08x] = r%d = %08x\n",
			       base + n, reg, v);
			STORE32(base + n, v);
			MFCHECK(0);
		}
		WRITE_GPR(reg, v);
		reg = (reg + 1) & 0x1f;
	}

        incPC();
        COUNT(CTR_INST_STSWX);
        INTERP_RETURN;
}
