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
#include <setjmp.h>

#include <functional>

#include "PPCCPUState.h"
#include "PPCMMU.h"
#include "PPCInterpreter.h"
#include "types.h"
#include "Config.h"
#include "blockstore.h"
#include "blockgen.h"

jmp_buf	runloop_restart; /* Todo: shove in an object, multithread-safe */

void runloop(PPCMMU *mmu, PPCInterpreter *interp, PPCCPUState *pcs)
{
	unsigned int instr_limit = CFG(instr_limit);
	unsigned int dsp = CFG(dump_state_period);
	bool break_runloop = false;

        pcs->setJmpbuf(&runloop_restart);
	/* The instruction routines longjmp back to here if they have an
	 * exception, i.e. force another block lookup.
	 */
	if (setjmp(runloop_restart)) {
		JITTRACE("Restarting runloop after exception\n");
	}

	if (break_runloop)
		return;

	while (!interp->breakRequested()) {
		// lookup by {PC, MSR.PR, MSR.DR, MSR.IR}, as changes to IR/DR
		// are fairly frequent so worth not invalidating blockcache.
		block_t *to;
		PPCMMU::fault_t fault;

	find_block:
		to = findBlock(mmu, pcs /* current CPU state */, &fault);
		if (!to) {
			if (fault != PPCMMU::FAULT_NONE) {
				JITTRACE("Fault %d\n", fault);
				pcs->raiseMemException(true, true, pcs->getPC(), fault);
				continue;
			} else {
				/* Otherise no block; make one */
				to = createBlock(mmu, pcs);
				if (!to)	/* PC faults, no block created */
					goto find_block;
				COUNT(CTR_JIT_BLOCKS_GEN);
			}
		}
		JITTRACE("Calling block %p, code at %p\n", to, BLOCK_CODEPTR(to));
		COUNT(CTR_JIT_BLOCKS_EXEC);

		/* Runs block. */
		block_fn_t c = (block_fn_t)BLOCK_CODEPTR(to);
		int r = c(interp);
		JITTRACE("Back, retval %d\n", r);

		/* Use returned nr of cycles consumed by block, to bump on the DEC/ticks: */
		pcs->CPUTick(r);
		if (pcs->isIRQPending()) {
			pcs->raiseIRQException();
		} else if (pcs->isDecrementerPending()) {
			pcs->raiseDECException();
		}

		// Reached limit for instructions?
		if (instr_limit && pcs->getCPUTicks() > instr_limit) {
			LOG("Hit instruction limit, quitting\n");
			interp->breakRequest();
		}
		// Debug/Instrumentation:
		if (dsp) {
			if ((pcs->getCPUTicks() % dsp) == 0)
				pcs->dump();
		}
	};

}
