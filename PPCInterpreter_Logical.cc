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

/********************* Logical *********************/

RET_TYPE PPCInterpreter::op_cntlzw(int RS, int RA, int Rc)
{
	// Inputs 'RS', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	REG val_RA;

	if (val_RS == 0)
		val_RA = 32;
	else
		val_RA = __builtin_clz(val_RS);	/* GCC-only? */

	DISASS("%08x   %08x  cntlzw%s\t %d,  %d  [r = %d]\n", pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, val_RA);

	WRITE_GPR(RA, val_RA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_CNTLZW);
	INTERP_RETURN;
}

static u32 mkmask(int MB, int ME)
{
	return MB <= ME ?
		( (0xffffffffu >> MB) & ~(0x7fffffffu >> ME) ) :
		( (0xffffffff << (31-ME)) | ~(0xfffffffeu << (31-MB)));
}

RET_TYPE PPCInterpreter::op_rlwimi(int RA, int RS, int SH, int MB, int ME, int Rc)
{
	// Inputs 'RA,RS,SH,MB,ME', implicit inputs ''
	REG val_RA = READ_GPR(RA);
	REG val_RS = READ_GPR(RS);

	u32 mask = mkmask(MB, ME);
	REG val = (val_RA & ~mask) | (ROTL32(val_RS, SH) & mask);

	DISASS("%08x   %08x  rlwimi%s\t %d,  %d, %d, %d, %d [ra = %08x, result = %08x, mask %08x]\n",
	       pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, /* in */ SH, /* in */ MB, /* in */ ME, val_RA, val, mask);
	val_RA = val;
	WRITE_GPR(RA, val_RA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_RLWIMI);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_rlwinm(int RS, int SH, int MB, int ME, int RA, int Rc)
{
	// Inputs 'RS,SH,MB,ME', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	REG val_RA;

	u32 mask = mkmask(MB, ME);
	REG val = ROTL32(val_RS, SH) & mask;

	DISASS("%08x   %08x  rlwinm%s\t %d,  %d, %d, %d, %d [result = %08x, mask %08x]\n",
	       pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, /* in */ SH, /* in */ MB, /* in */ ME, val, mask);
	val_RA = val;
	WRITE_GPR(RA, val_RA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_RLWINM);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_rlwnm(int RB, int RS, int MB, int ME, int RA, int Rc)
{
	// Inputs 'RB,RS,MB,ME', implicit inputs ''
	REG val_RB = READ_GPR(RB);
	REG val_RS = READ_GPR(RS);
	REG val_RA;

	u32 mask = mkmask(MB, ME);
	REG val = ROTL32(val_RS, val_RB & 0x1f) & mask;

	DISASS("%08x   %08x  rlwnm%s\t %d,  %d, %d, %d, %d [result = %08x, rs_shift = %d, mask %08x]\n",
	       pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, /* in */ RB, /* in */ MB, /* in */ ME, val, val_RB & 0x1f, mask);
	val_RA = val;
	// Outputs 'RA', implicit outputs ''
	WRITE_GPR(RA, val_RA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_RLWNM);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_sraw(int RS, int RB, int RA, int Rc)
{
	// Inputs 'RS,RB', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	REG val_RB = READ_GPR(RB);
	REG val_RA;
	int val_CA;

	DISASS("%08x   %08x  sraw%s\t %d,  %d, %d \n", pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, /* in */ RB);

	int sh = val_RB & 0x3f;
	if (sh > 31) {
		val_RA = ((s32)val_RS) >> 31; /* Sign everywhere */
		val_CA = !!(val_RS & 0x80000000);
	} else {
		val_RA = ((s32)val_RS) >> sh;
		val_CA = (val_RS & 0x80000000) && (val_RS & ((1 << sh)-1));
	}

	// Outputs 'RA', implicit outputs 'CA'
	WRITE_GPR(RA, val_RA);
	WRITE_CA(val_CA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_SRAW);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_srawi(int RS, int SH, int RA, int Rc)
{
	// Inputs 'RS,SH', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	REG val_RA;
	int val_CA;

	DISASS("%08x   %08x  srawi%s\t %d,  %d, %d \n", pc, inst, (Rc ? "." : ""), /* out */ RA, /* in */ RS, /* in */ SH);

	val_RA = ((s32)val_RS) >> SH;
	val_CA = (val_RS & 0x80000000) && (val_RS & ((1 << SH)-1));

	// Outputs 'RA', implicit outputs 'CA'
	WRITE_GPR(RA, val_RA);
	WRITE_CA(val_CA);

	if (Rc) {
		SET_CR0(val_RA); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_SRAWI);
	INTERP_RETURN;
}

