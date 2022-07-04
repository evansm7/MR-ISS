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

#include "OS.h"
#include <arpa/inet.h> // For htonl
#include <unistd.h>

static OS	os;

void	OS::push32(u32 val)
{
	cur_stack_ptr -= 4;
	*((u32 *)to_host(cur_stack_ptr)) = htonl(val);
	DEBUG("Stack:  %08x = (BE) %08x\n", cur_stack_ptr, val);
}

void	OS::push8(u8 val)
{
	cur_stack_ptr -= 1;
	*((u8 *)to_host(cur_stack_ptr)) = val;
	DEBUG("Stack:  %08x = %02x\n", cur_stack_ptr, val);
}

void	OS::init(PPCCPUState *pcs, ADR ram_base, ADR ram_size, u8 *real_ram_base)
{
	this->ram_base = ram_base;
	this->ram_top = ram_base + ram_size - 1;
	this->real_ram_base = real_ram_base;

	cur_stack_ptr = ram_base + ram_size - 16;

	// Random bytes -- to stop dietlibc opening /dev/random...
	push32(0xdeadbeef);
	push32(0xfeedface);
	push32(0xcafeb00c);
	push32(0xbaadbeef);
	VA rand = cur_stack_ptr;

	// Create stackframe.
	// This is *just* enough to get PPC diet-libc to start up:

	// First, args:
	push8(0);
	push8('o');
	push8('o');
	push8('f');	// program name 'foo'
	VA argp = cur_stack_ptr;

	// Dummy env!
	push32(0);
	push8(0);
	push8('1');
	push8('=');
	push8('F');	// program name 'foo'
	VA envp = cur_stack_ptr;

	// Set up AUXV (in reverse order!):
	// Terminator at top:
	push32(0);	//
	push32(0);	// AT_NULL
	// 33 for VDSO; hoping I don't need a VDSO for diet-libc...
	push32(rand);
	push32(25);	// AT_RANDOM address
	VA auxv = cur_stack_ptr;

	push32(0);
	push32(envp);	// Array of pointers to env
	VA envv = cur_stack_ptr;

	push32(0);
	push32(argp);	// Array of pointers to args
	VA argv = cur_stack_ptr;

	push32(1);	// argc

	// As yet unused vars:
	envv = 0;
	argv = 0;
	auxv = 0;

        /* Stack pointer in r1: */
	pcs->setGPR(1, cur_stack_ptr);
}

/* Handle syscalls:
 *
 * Instead of making this elaborate (it will be huge and difficult), make the test program simpler...
 */
void	OS::doSyscall(PPCCPUState *pcs, unsigned int LEV)
{
	unsigned int sc = pcs->getGPR(0);
	STRACE("Syscall %d ->\n", sc);
	REG a0 = pcs->getGPR(3);
	REG a1 = pcs->getGPR(4);
	REG a2 = pcs->getGPR(5);
	REG a3 = pcs->getGPR(6);
	REG a4 = pcs->getGPR(7);
	REG a5 = pcs->getGPR(8);
	REG a6 = pcs->getGPR(9);
	REG a7 = pcs->getGPR(10);
	REG result = 0;
	bool err = false;

	/* Not all args are used yet, suppress warnings. */
	(void)a4;
	(void)a5;
	(void)a6;
	(void)a7;

	switch (sc) {
		case 1:	{	// NR_exit
			STRACE("sc_exit\n");
			FATAL("Syscall exit terminated sim, rc = %d\n", a0);
		} break;
		case 4: {	// NR_write
			STRACE("sc_write(%d, 0x%x, 0x%x)\n", a0, a1, a2);
			switch (a0) {
				case 1: 	// stdout
				case 2: {	// stderr
					if (!in_mem(a1) || !in_mem(a1+a2)) {
						// FIXME: flag EFAULT
					} else {
						result = write(1, to_host(a1), a2);
					}
				} break;
				default:
					FATAL("write() to fd %d unsupported\n", a0);
			}
			result = a2;
		} break;
		case 54: {	// NR_ioctl
			STRACE("sc_ioctl(%d, 0x%x, 0x%x)\n", a0, a1, a2);
			// Um... pretend it worked...?
			result = 0;
		} break;
		case 173: { 	// NR_rtsigaction
			STRACE("sc_rtsigaction(%d, 0x%x, 0x%x, 0x%x)\n", a0, a1, a2, a3);
			// Fake! SAD.
			result = 0;
		} break;
		default:
			FATAL("Syscall %d unsupported\n", sc);
			break;
	}

	u32 cr = pcs->getCR();
	if (err)
		cr |= 0x10000000;	/* Set SO */
	else
		cr &= ~0x10000000;	/* Clear SO */
	pcs->setCR(cr);

        pcs->setGPR(3, result);
	STRACE("Syscall <- 0x%08x\n", result);
}

OS	*getOS()
{
	return &os;
}
