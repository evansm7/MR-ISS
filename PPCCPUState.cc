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

#include "PPCCPUState.h"
#include "log.h"

#define INSTR_LOG_HUMAN_READABLE yes


PPCCPUState::PPCCPUState()
{
	for (int i = 0; i < 32; i++) {
		gprs[i] = 0;
	}
	pc = ctr = lr = 0;
	xer = 0;
	cr = 0;
	msr = 0;
	pir = 0;
	tb = 0;
	cpu_inst_count = 0;
	hid0 = 0;
	hid1 = 0;
	dar = 0;
	dsisr = 0;
	dec = 0xffffffff;
	irqFlag = false;

	mmu = 0;
	exception_jmp = 0;
}

void	PPCCPUState::dump()
{
#ifdef INSTR_LOG_HUMAN_READABLE
	LOG("CPU[%d] state:   Icount %016lx\n", pir, cpu_inst_count);
	LOG(" PC  " FMT_REG "   LR  " FMT_REG
	    "   CTR " FMT_REG
	    "   CR  %08x   XER %016x"
	    "          MSR %08x   DEC %08x\n",
	    pc, lr,
	    ctr,
	    cr, xer, msr, dec);
	for (int i = 0; i < 32; i++) {
		if ((i & 7) == 0)
			LOG(" ");
		LOG("r%02d " FMT_REG "   ", i, getGPR(i));
		if ((i & 7) == 7)
			LOG("\n");
	}
#else
	/* This style of log is the same as that output by the
	 * ppc_trace utility, so can be compared against it
	 * directly.
	 */
	LOG(FMT_REG " ", pc);
	for (int i = 0; i < 32; i++) {
		LOG(FMT_REG " ", getGPR(i));
	}
	LOG(FMT_REG " " FMT_REG " " FMT_REG " " FMT_REG "\n",
	    lr, ctr, cr, xer);
#endif
}

/* Render a machine-readable version of the CPU state */
void	PPCCPUState::render_mr(char *buffer, size_t len)
{
	int r;
	size_t s = len;
	char *p = buffer;

	r = snprintf(p, s,
		     "ICOUNT %016llx, PC " FMT_REG ", "
		     "LR " FMT_REG ", CTR " FMT_REG ", CR " FMT_REG ", "
		     "XER " FMT_REG ", MSR " FMT_REG ", DEC " FMT_REG ", ",
		     cpu_inst_count, pc, lr, ctr, cr, xer, msr, dec);
	if (r < 0)
		return;
	s -= r;
	p += r;

	for (int i = 0; i < 32; i++) {
		r = snprintf(p, s, "r%02d " FMT_REG ", ", i, getGPR(i));
		if (r < 0)
			return;
		s -= r;
		p += r;
	}
	/* Force a newline at the end */
	if (s <= 1) {
		p = &buffer[len-2];
	}
	*p++ = '\n';
	*p = '\0';
}

/* This fn munges MSR and PC, SRR0/1, to deliver exception e.
 * It assumes that exception-related regs like DSISR/DAR are set up
 * already.
 *
 * FIXME: That might be best to sort out in here.  Reconsider.
 * Note: Exceptions don't blow reservations.
 */
void	PPCCPUState::takeException(exception_t e, REG new_srr1)
{
	srr0 = pc & ~3;
	srr1 = new_srr1;

	EXCTRACE("Exception 0x%x at PC=%08x, MSR=%08x, SRR1=%08x\n",
		 e, pc, msr, srr1);
	// See PEM sec 6.4.  LE copied from ILE, ME generally unchanged (except for MC exception), ILE unchanged.
	msr = msr & MSR_IP; // FIXME will be OK for now, but will break little endian!
	pc = ((msr & MSR_IP) ? 0xfff00000 : 0) + (PA)e;

	/* Update MMU's view of IR/DR: */
	mmu->setIRDR(getMSR() & MSR_IR, getMSR() & MSR_DR);

	/* "A little hacky, but..."  If JIT, then break out of the current
	 * block.  Return from here to the start of the runloop:
	 */
	if (exception_jmp) {
		longjmp(*exception_jmp, 1);
	}
}

void	PPCCPUState::raiseMemException(bool RnW, bool InD, VA addr, PPCMMU::fault_t fault, u32 inst)
{
	COUNT(CTR_EXCEPTION_MEM);

	EXCTRACE("-> Handle exception %d from mem addr %p, RnW %d, InD %d\n",
		 fault, addr, RnW, InD);
	if (log_enables_mask & LOG_FLAG_EXC_TRACE) {
		dump();
	}

	exception_t exc = EXC_RESET;
	REG srr1 = getMSR();

	switch (fault) {
		case PPCMMU::FAULT_NO_PAGE:
			if (InD) {
				exc = EXC_ISI;
				srr1 &= ~0xf8000000;
				srr1 |= 0x40000000; // Cause: Page fault
			} else {
				exc = EXC_DSI;
				setDAR(addr);
				setDSISR(
					0x40000000 | // Cause: DR and page fault
					(RnW ? 0 : 0x02000000)
					);
				/* Wasn't caused by:
				   - Data watchpoint
				   - atomic/quad to guarded
				   - class key
				   - caching inhibited
				 */
			}
			break;

		case PPCMMU::FAULT_PERMS:
			if (InD) {
				exc = EXC_ISI;
				srr1 &= ~0xf8000000;
				srr1 |= 0x08000000; // Cause: Page fault
			} else {
				exc = EXC_DSI;
				setDAR(addr);
				setDSISR(
					0x08000000 | // Permission fault
					(RnW ? 0 : 0x02000000)
					);
				/* Wasn't caused by:
				   - Data watchpoint
				   - atomic/quad to guarded
				   - class key
				   - caching inhibited
				 */
			}
			break;

		case PPCMMU::FAULT_PERMS_NX:
			ASSERT(InD);
			exc = EXC_ISI;
			srr1 &= ~0xf8000000;
			srr1 |= 0x10000000; // Cause: NX segment
			break;

                case PPCMMU::FAULT_ALIGN:
                        exc = EXC_ALIGN;
                        setDAR(addr);
                        setDSISR( ((inst & 0x80000000) ?
                                   (((inst >> (26-14)) & 0x4000) |
                                    ((inst >> (27-10)) & 0x3c00))
                                   :
                                   (((inst << (15-1)) & 0x18000) |
                                    ((inst << (14-6)) & 0x4000) |
                                    ((inst << (10-7)) & 0x3c00))) |
                                  ((inst >> (21-5)) & 0x3e0) |
                                  ((inst >> 16) & 0x1f) );
                        break;

		default:
			FATAL("%s: Unhandled fault type %d on address %p\n",
			      __PRETTY_FUNCTION__, fault, addr);
	}

	takeException(exc, srr1);
}

void	PPCCPUState::raiseIRQException()
{
	COUNT(CTR_EXCEPTION_IRQ);

	EXCTRACE("Delivering External IRQ\n");
	if (log_enables_mask & LOG_FLAG_EXC_TRACE) {
		dump();
	}
	takeException(EXC_IRQ, getMSR());
}

void	PPCCPUState::raiseDECException()
{
	COUNT(CTR_EXCEPTION_DEC);

	EXCTRACE("Delivering DEC exception\n");
	takeException(EXC_DEC, getMSR());
}

void	PPCCPUState::raisePROGException(u32 reason)
{
	COUNT(CTR_EXCEPTION_PROG);

	EXCTRACE("Delivering DEC exception\n");
	takeException(EXC_PROG, (getMSR() & 0x8000ffff) | reason);
}

void	PPCCPUState::raiseSCException()
{
	COUNT(CTR_EXCEPTION_SC);

	EXCTRACE("Delivering SC exception\n");
	takeException(EXC_SC, getMSR());
}
