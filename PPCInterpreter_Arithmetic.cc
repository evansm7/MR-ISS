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

/********************* Arithmetic *********************/

RET_TYPE PPCInterpreter::op_divw(int RA, int RB, int RT, int Rc, int OE)
{
	// Inputs 'RA,RB', implicit inputs ''
	REG val_RA = READ_GPR(RA);
	REG val_RB = READ_GPR(RB);
	sREG val_RT;
	int val_OV = 0;

	DISASS("%08x   %08x  divw%s%s\t %d,  %d, %d \n", pc, inst, (OE ? "o" : ""), (Rc ? "." : ""), /* out */ RT, /* in */ RA, /* in */ RB);

#ifdef NO_DIVIDE
	cpus->raisePROGException(0x80000);
#else
	if (val_RB == 0) {
		val_RT = val_RA >= 0x80000000 ? 0x80000000 : 0x7fffffff;	/* A la 603 impl. */
		val_OV = 1;
	} else if (val_RA == 0x80000000 && val_RB == 0xffffffff) {
		val_RT = 0x7fffffff;						/* A la 603 impl. */
		val_OV = 1;
	} else {
		val_RT = (sREG)val_RA / (sREG)val_RB;	/* Does this round weird cos it's IBM? */
	}
	WRITE_GPR(RT, val_RT);

	if (OE) {
		SET_SO_OV(val_OV);
	}
	if (Rc) {
		SET_CR0(val_RT); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_DIVW);
#endif
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_divwu(int RA, int RB, int RT, int Rc, int OE)
{
	// Inputs 'RA,RB', implicit inputs ''
	REG val_RA = READ_GPR(RA);
	REG val_RB = READ_GPR(RB);
	REG val_RT = 0xffffffff;
	int val_OV = 0;

	DISASS("%08x   %08x  divwu%s%s\t %d,  %d, %d \n", pc, inst, (OE ? "o" : ""), (Rc ? "." : ""), /* out */ RT, /* in */ RA, /* in */ RB);

#ifdef NO_DIVIDE
	cpus->raisePROGException(0x80000);
#else
	if (val_RB == 0) {
		val_OV = 1;
	} else {
		val_RT = val_RA / val_RB;	/* Does this round weird cos it's IBM? */
	}
	WRITE_GPR(RT, val_RT);

	if (OE) {
		SET_SO_OV(val_OV);
	}
	if (Rc) {
		SET_CR0(val_RT); // Copies XER.SO
	}
	incPC();
	COUNT(CTR_INST_DIVWU);
#endif
	INTERP_RETURN;
}

