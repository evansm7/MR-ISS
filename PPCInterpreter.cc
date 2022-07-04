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

void	*PPCInterpreter::op_unk(u32 inst)
{
	COUNT(CTR_INST_UNK);
#ifdef HOSTED
	cpus->dump();
	FATAL("Unknown instruction %08x at PC %08x\n", inst, cpus->getPC());
#else
	EXCTRACE("-> Unknown instruction %08x at PC %08x, Program interrupt\n", inst, cpus->getPC());
	cpus->raisePROGException(0x80000);
#endif
	return 0;
}

void    PPCInterpreter::WRITE_MSR(REG32 val)
{
	cpus->setMSR(val);
	mmu->setIRDR(val & MSR_IR, val & MSR_DR);
}
