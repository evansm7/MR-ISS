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

/********************* CondReg *********************/

RET_TYPE PPCInterpreter::op_mcrf(int BF, int BFA)
{
        uint32_t val_CR = READ_CR();
	DISASS("%08x   %08x  mcrf\t %d, %d \n", pc, inst, /* in */ BF, /* in */ BFA);

	WRITE_CR_FIELD(val_CR, BF, READ_CR_FIELD(val_CR, BFA));
	WRITE_CR(val_CR);

	incPC();
	COUNT(CTR_INST_MCRF);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mcrxr(int BF)
{
        uint32_t val_CR = READ_CR();
	DISASS("%08x   %08x  mcrxr\t  \n", pc, inst);

	WRITE_CR_FIELD(val_CR, BF, cpus->getXER() >> 28);
	WRITE_CR(val_CR);
	cpus->setXER(0);

	incPC();
	COUNT(CTR_INST_MCRXR);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mfcr(int RT)
{
        // Inputs '', implicit inputs 'CR'
        uint32_t val_CR = READ_CR();
        REG val_RT;

        DISASS("%08x   %08x  mfcr\t %d [CR = %08x]\n", pc, inst, /* out */ RT, val_CR);

	val_RT = val_CR;

        // Outputs 'RT', implicit outputs ''
        WRITE_GPR(RT, val_RT);

        incPC();
        COUNT(CTR_INST_MFCR);
        INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mtcrf(int FXM, int RS)
{
	// Inputs 'FXM,RS', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	uint32_t val_CR = READ_CR();

	uint32_t mask = ((FXM & 0x80) ? 0xf0000000 : 0) |
		((FXM & 0x40) ? 0x0f000000 : 0) |
		((FXM & 0x20) ? 0x00f00000 : 0) |
		((FXM & 0x10) ? 0x000f0000 : 0) |
		((FXM & 0x08) ? 0x0000f000 : 0) |
		((FXM & 0x04) ? 0x00000f00 : 0) |
		((FXM & 0x02) ? 0x000000f0 : 0) |
		((FXM & 0x01) ? 0x0000000f : 0);
	
	DISASS("%08x   %08x  mtcrf\t %d, %d  [mask = %08x]\n", pc, inst, /* in */ FXM, /* in */ RS, mask);
	val_CR = (val_CR & ~mask) | (val_RS & mask);

	// Outputs '', implicit outputs 'CR'
	WRITE_CR(val_CR);

	incPC();
	COUNT(CTR_INST_MTCRF);
	INTERP_RETURN;
}

