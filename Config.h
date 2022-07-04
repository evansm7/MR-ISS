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

#ifndef CONFIG_H
#define CONFIG_H
#include "types.h"
#include "PPCCPUState.h"
#include "platform.h"


#define CFG(x)	(getConfig()->x)

class Config {
public:
	//////////////////////////////////// CONFIG ITEMS //////////////////////////////
	bool		verbose;
	bool		disass;
	u64 		instr_limit;
	unsigned int 	dump_state_period;
	char 		*rom_path;
	PA		load_addr;
	ADR		start_addr;
	bool		strace;
	bool		io_trace;
	bool		branch_trace;
	bool		mmu_trace;
	bool		exception_trace;
	bool		jit_trace;
	char		*block_path[SBD_NUM];
	u32		start_msr;
	char 		*save_state_path;
#if PLATFORM == 3
        u32             gpio_inputs;
#endif

	////////////////////////////////////////////////////////////////////////////////
        Config() :
	        verbose(false)
		,disass(false)
		,instr_limit(0)
		,dump_state_period(0)
		,rom_path((char *)"rom.bin")
		,load_addr(0)
		,start_addr(~0 /* See Config.cc */)
		,strace(false)
		,io_trace(false)
		,branch_trace(false)
		,mmu_trace(false)
		,exception_trace(false)
		,jit_trace(false)
		,start_msr(RESET_MSR_VAL)
		,save_state_path(0)
#if PLATFORM == 3
                ,gpio_inputs(0x80000000)
#endif
	{
		for (int i = 0; i < SBD_NUM; i++)
			block_path[i] = 0;
	}

	void	setup(int argc, char *argv[]);
private:
	void	usage(char *prog);
};

extern Config	*getConfig();

#endif
