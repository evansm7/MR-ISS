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

#include "types.h"
#include "log.h"
#include "Config.h"
#include "stats.h"
#include "OS.h"
#include "management.h"
#include "Bus.h"
#include "platform.h"
#include "PPCInterpreter.h"
#include "blockstore.h"
#include "runloop.h"
#include "sim_state.h"


////////////////////////////////////////////////////////////////////////////////
// Memory system
Bus		bus;

////////////////////////////////////////////////////////////////////////////////
// CPUs:
PPCCPUState	pcs;	// UP for now.

////////////////////////////////////////////////////////////////////////////////

static void	set_initial_cpu_context(PPCCPUState *pcs)
{
	pcs->setPC(CFG(start_addr));
	pcs->setMSR(CFG(start_msr));
	pcs->setPIR(0);

#ifdef HOSTED
	ADR ram_pa_base;
	u32 ram_size;
	u8 *host_map_base;
	platform_meminfo(&ram_pa_base, &ram_size, &host_map_base);
	getOS()->init(pcs, ram_pa_base, ram_size, host_map_base);
#endif
}

void 	sim_quit(void)
{
	printf("Shutting down.\n\n");
	pcs.dump();
	stats_dump();

        meminfo_t mi[PLAT_MEMINFO_MAX_BANKS];
        unsigned int banks = platform_meminfo(mi);
        int ssh = state_save_open(&pcs, mi, banks);
        if (ssh >= 0) {
                platform_state_save(ssh);
                state_save_finalise(ssh);
        }

	exit(1);
}

int 	main(int argc, char *argv[])
{
	////////////////////////////// General init

	getConfig()->setup(argc, argv);

	log_init();
	LOG("Log init\n");

	management_init();

	////////////////////////////// Set up memory system and IO:
	DEBUG("Initialising platform\n");
	platform_init(&bus);

	////////////////////////////// Make one CPU context:

	set_initial_cpu_context(&pcs);

	LOG("Initial CPU state:\n");
	pcs.dump();

	// Attach IRQ controller to CPU state (to assert IRQ):
	platform_irq_cpuclient(&pcs);

	bus.dump();

	// There is one interpreter per thread.  Interpreter 'contains' the CPU
	// state.
	PPCInterpreter 	interp;
	interp.setCPUState(&pcs);

	PPCMMU		mmu;
	mmu.setBus(&bus);
	interp.setMMU(&mmu);
	pcs.setMMU(&mmu);

	LOG("----\n\n");
	// Fixme:
	// set endian
	// set halt on UNDEF
	// set halt on exception

#if ENABLE_JIT == 0
	unsigned int instr_limit = CFG(instr_limit);
	unsigned int dsp = CFG(dump_state_period);

	while (!interp.breakRequested()) {
		interp.execute();
		// Print PC/state?
		// Apply debug/breakpoint activities?
		// Poll devices?
		pcs.CPUTick();
		if (pcs.isIRQPending()) {
			pcs.raiseIRQException();
		} else if (pcs.isDecrementerPending()) {
			pcs.raiseDECException();
		}

		// Reached limit for instructions?
		if (instr_limit && pcs.getCPUTicks() > instr_limit) {
			LOG("Hit instruction limit, quitting\n");
			interp.breakRequest();
		}
		// Debug/Instrumentation:
		if (dsp) {
			if ((pcs.getCPUTicks() % dsp) == 0)
				pcs.dump();
		}
		platform_poll_periodic(pcs.getCPUTicks());
	};
#else
	initBlockStore();
	runloop(&mmu, &interp, &pcs);
#endif

	sim_quit();
	return 0;
}
