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

/* A trivial OS layer to initialise "process" stack and handle syscalls.
 *
 * If this gets complex, something has gone wrong. somewhere...  ;-)
 * ME 210118
 */

#ifndef OS_H
#define OS_H

#include "types.h"
#include "PPCCPUState.h"
#include "log.h"

class OS
{
public:
        OS() : ram_base(~0), ram_top(~0), cur_stack_ptr(~0), real_ram_base(0)
	{}

	void	init(PPCCPUState *pcs, VA ram_base, VA ram_size, u8 *real_ram_base);

	void	doSyscall(PPCCPUState *pcs, unsigned int LEV);

private:
	bool	in_mem(VA sim_addr)
	{
		return (sim_addr >= ram_base) && (sim_addr <= ram_top);
	}

	uint8_t *to_host(VA sim_addr)
	{
		if (!in_mem(sim_addr)) {
			FATAL("Address %x out of range (base %d, top %d)\n",
			      sim_addr, ram_base, ram_top);
		}
		return real_ram_base + sim_addr - ram_base;
	}

	void	push32(u32 val);
	void	push8(u8 val);

	VA	ram_base;
	VA	ram_top;	/* Inclusive */

	VA	cur_stack_ptr;

	u8 	*real_ram_base;
};

extern OS	*getOS();

#endif


