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

#ifndef	HW_H
#define HW_H

#define	B(X)			(1<<(X))

// Memory map, a simple subset of the MR3 platform:

#define MAIN_RAM_PHYS_ADDR   	0x00000000
#define MAIN_RAM_SIZE   	(2*1024*1024)
// Stack starts at mem top... give it a little breathing space after the FB:
#define FB_BASE			(MAIN_RAM_SIZE - 512*1024)

// CPU

#define CPU_CACHE_CL		32
#define CPU_CACHE_SETS		(16384/CPU_CACHE_CL/4)	// 16KB 4-way assoc

// IO

#define IO_BASE                 0x82000000
#define UART_PHYS_ADDR		(IO_BASE + 0x00000)
#define LCDC0_PHYS_ADDR		(IO_BASE + 0x40000)


#endif
