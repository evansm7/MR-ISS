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
#include "OS.h"

/********************* Trap *********************/

RET_TYPE PPCInterpreter::op_sc(uint8_t LEV)
{
	DISASS("%08x   %08x  sc\t %d \n", pc, inst, /* in */ LEV);

#ifdef HOSTED
	getOS()->doSyscall(cpus, LEV);
	incPC();
#else
        STRACE("PC %08x: sc(%4d): args=%08x %08x %08x %08x\n", pc,
               READ_GPR(0), READ_GPR(3), READ_GPR(4), READ_GPR(5), READ_GPR(6));
	incPC();
	cpus->raiseSCException();
#endif
	COUNT(CTR_INST_SC);
	INTERP_RETURN;
}

static bool does_trap(int TO, s32 a, s32 b)
{
	return ((TO & 0x10) && (a < b)) ||
		((TO & 0x08) && (a > b)) ||
		((TO & 0x04) && (a == b)) ||
		((TO & 0x02) && ((u32)a < (u32)b)) ||
		((TO & 0x01) && ((u32)a > (u32)b));
}

RET_TYPE PPCInterpreter::op_tw(int TO, int RA, int RB)
{
	// Inputs 'TO,RA,RB', implicit inputs ''
	REG val_RA = READ_GPR(RA);
	REG val_RB = READ_GPR(RB);

	COUNT(CTR_INST_TW);

	DISASS("%08x   %08x  tw\t %d, %d, %d \n", pc, inst, /* in */ TO, /* in */ RA, /* in */ RB);

	if (does_trap(TO, val_RA, val_RB)) {
#ifdef HOSTED
		FATAL("tw trap at %08x\n", pc);
#else
		cpus->raisePROGException(0x20000);
		INTERP_RETURN;
#endif
	}

	incPC();
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_twi(int TO, int RA, int16_t SI)
{
	// Inputs 'TO,RA,SI', implicit inputs ''
	REG val_RA = READ_GPR(RA);

	COUNT(CTR_INST_TWI);

	DISASS("%08x   %08x  twi\t %d, %d, %d \n", pc, inst, /* in */ TO, /* in */ RA, /* in */ SI);

	if (does_trap(TO, val_RA, (int32_t)SI)) {
#ifdef HOSTED
		FATAL("twi trap at %08x\n", pc);
#else
		cpus->raisePROGException(0x20000);
		INTERP_RETURN;
#endif
	}

	incPC();
	INTERP_RETURN;
}

