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

/********************* Branch *********************/

static uint32_t branch_count = 0;

RET_TYPE PPCInterpreter::op_b(uint32_t LI, int AA, int LK)
{
	const char *opc[] = { "b", "bl", "ba", "bla" };
	int32_t offset = ((int32_t)(LI << 6) >> 6);
	VA	dest;
	if (AA)
		dest = offset;
	else
		dest = cpus->getPC() + offset;

	if (dest == cpus->getPC()) {
		// Todo: And, IRQs disabled (including HV...)
		LOG("\n\nBranch to self, quitting.\n");
		SIM_QUIT(0);
	}

	if (LK)
		cpus->setLR(cpus->getPC() + 4);

	cpus->setPC(dest);

	DISASS("%08x   %08x  %s\t\t 0x%08x  [LI %08x]\n", pc, inst, opc[ (LK ? 1 : 0) | (AA ? 2 : 0) ], dest, /* in */ LI);
	BRTRACE("+++ %08x  Branch dest %08x\n", branch_count++, cpus->getPC()); /* Trace state after branch */

	COUNT(CTR_INST_B);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_bc(int BO, int BI, int BD, int AA, int LK)
{
	const char *opc[] = { "bc", "bcl", "bca", "bcla" };

	// bi is cr bit to test (top-down)
	bool	cr_bit 	= getCRb(BI);

	bool	eq	= BO & 0x02;	// If true, test ctr == 0 else != 0
	bool	n_ctr	= BO & 0x04;	// If true, don't touch or test ctr
	bool	cr_p	= BO & 0x08;
	bool	cr_al	= BO & 0x10;	// If true, don't care about condition reg

	bool	branch	= cr_al || (cr_bit == cr_p);	// Does condition (+override) eval true?
	if (!n_ctr) {
		// As well as the condition above, we need the (decremented) CTR to be zero/non-zero.
		REG	ctr = cpus->getCTR();
		ctr--;
		cpus->setCTR(ctr);
		branch = branch && ((ctr == 0) == eq);
	}

	int32_t offset = (int32_t)(int16_t)BD;		/* SXT.  NB: Parameter not shifted */
	VA	dest;
	if (AA)
		dest = offset;
	else
		dest = cpus->getPC() + offset;

	if (LK) {
		/* Unusually, PPC writes LR even on a not-taken branch! */
		cpus->setLR(cpus->getPC() + 4);
	}

	if (branch) {
		cpus->setPC(dest);
	} else {
		incPC();
	}

	DISASS("%08x   %08x  %s\t\t 0x%08x  [%s  %d, %d,  c%d z%d p%d nc%d a%d]\n",
	       pc, inst,
	       opc[ (LK ? 1 : 0) | (AA ? 2 : 0) ],
	       dest, branch ? "T" : "NT",
	       /* in */ BO, /* in */ BI,
	       cr_bit, eq, cr_p, n_ctr, cr_al);
	BRTRACE("+++ %08x  Branch dest %08x %s\n", branch_count++, cpus->getPC(), branch ? "" : "NT"); /* Trace state after branch */

	COUNT(CTR_INST_BC);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_bclr(int BO, int BI, int BH, int LK)
{
	// bi is cr bit to test (top-down)
	bool	cr_bit 	= getCRb(BI);

	bool	eq	= BO & 0x02;
	bool	n_ctr	= BO & 0x04;
	bool	cr_p	= BO & 0x08;
	bool	cr_al	= BO & 0x10;

	bool	branch	= cr_al || (cr_bit == cr_p);	// Does condition (+override) eval true?
	if (!n_ctr) {
		// As well as the condition above, we need the (decremented) CTR to be zero/non-zero.
		REG	ctr = cpus->getCTR();
		ctr--;
		cpus->setCTR(ctr);
		branch = branch && ((ctr == 0) == eq);
	}

	VA	dest = cpus->getLR() & ~3;
	if (LK)
		cpus->setLR(cpus->getPC() + 4);

	if (branch) {
		cpus->setPC(dest);
	} else {
		incPC();
	}

	DISASS("%08x   %08x  %s\t\t 0x%08x  [%s  %d, %d,  c%d z%d p%d nc%d a%d]\n",
	       pc, inst,
	       LK ? "bclrl" : "bclr",
	       dest, branch ? "T" : "NT",
	       BO, BI,
	       cr_bit, eq, cr_p, n_ctr, cr_al);
	BRTRACE("+++ %08x  Branch dest %08x %s\n", branch_count++, cpus->getPC(), branch ? "" : "NT"); /* Trace state after branch */

	COUNT(CTR_INST_BCLR);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_bcctr(int BO, int BI, int BH, int LK)
{
	// bi is cr bit to test (top-down)
	bool	cr_bit 	= getCRb(BI);

	bool	eq	= BO & 0x02;
	bool	n_ctr	= BO & 0x04;
	bool	cr_p	= BO & 0x08;
	bool	cr_al	= BO & 0x10;

	bool	branch	= cr_al || (cr_bit == cr_p);	// Does condition (+override) eval true?
	if (!n_ctr) {
		// This form is not valid, thankfully.
		return op_unk(inst);
	}

	if (LK)
		cpus->setLR(cpus->getPC() + 4);

	VA	dest = cpus->getCTR() & ~3;
	if (branch) {
		cpus->setPC(dest);
	} else {
		incPC();
	}

	DISASS("%08x   %08x  %s\t\t 0x%08x  [%s  %d, %d,  c%d z%d p%d a%d]\n",
	       pc, inst,
	       LK ? "bcctrl" : "bcctr",
	       dest, branch ? "T" : "NT",
	       BO, BI,
	       cr_bit, eq, cr_p, cr_al);
	BRTRACE("+++ %08x  Branch dest %08x %s\n", branch_count++, cpus->getPC(), branch ? "" : "NT"); /* Trace state after branch */

	COUNT(CTR_INST_BCCTR);
	INTERP_RETURN;
}

