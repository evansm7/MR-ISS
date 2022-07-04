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

#ifndef PLATFORM_H
#define PLATFORM_H

#include "utility.h"


////////////////////////////////////////////////////////////////////////////////
// Types/defs

#define PLAT_MEMINFO_MAX_BANKS  8

typedef struct {
        ADR ram_pa_base;
        u32 ram_size;
        u8 *host_map_base;
} meminfo_t;

////////////////////////////////////////////////////////////////////////////////
// Functions

void	platform_init(Bus *bus);
void 	platform_irq_cpuclient(PPCCPUState *pcs);
unsigned int	platform_meminfo(meminfo_t info_out[]);

////////////////////////////////////////////////////////////////////////////////
// Platform-specific defs:

////////////////////////////// MR1:	////////////////////////////////////////

#if PLATFORM == 1
/* We don't have ROM, for now.  Much easier to just have one wodge of initialised RAM: */
#define RAM_BASE_ADDR	0x00000000
#define RAM_SIZE	MiB(512)

// IO devices have bit A[31]=1:
#define UART_BASE_ADDR	0x80000000
#define UART_SIZE	0x00000200

#define INTC_BASE_ADDR	0x80010000
#define INTC_SIZE	0x00000200

#define SBD0_BASE_ADDR	0x80020000
#define SBD_SIZE	0x00000020
#define SBD_NUM		4
#define SBD_STRIDE	0x100

#define UART0_IRQ	0
#define SBD0_IRQ	1
#define SBD_IRQ_STRIDE	1


static inline void	platform_poll_periodic(u64 ticks)
{
	// Nothing, no overhead.
}

static inline void      platform_state_save(int handle)
{
}

#endif

////////////////////////////// MR2:	////////////////////////////////////////

#if PLATFORM == 2
/* 512K blockRAM/boot memory at 0: */
#define RAM_BASE_ADDR	0x00000000
#define RAM_SIZE	KiB(512)

// IO devices have bit A[31]=1:
#define UART_BASE_ADDR	0x80000000
#define UART_SIZE	0x00000200

#define LCDC0_BASE_ADDR	0x80010000
#define LCDC0_SIZE	0x00000200

/* Periodic callbacks: */
void platform_poll_lcdc0(u64 ticks);
#define LCDC0_POLL_PERIOD_MASK	0xfffff

static inline void	platform_poll_periodic(u64 ticks)
{
	if ((ticks & LCDC0_POLL_PERIOD_MASK) == 0)
		platform_poll_lcdc0(ticks);
}

static inline void      platform_state_save(int handle)
{
}

#endif

#if PLATFORM == 3

/* 16MB SRAM at 0,
 * 16MB SRAM at 16M, (will eventually need 2nd bank, tho ofc this could be 1 contiguous)
 * IO at 2G,
 * plus 512K BRAM/boot memory at 0xfff00000
 */
#define RAM_BASE_ADDR		0x00000000
#define RAM_SIZE		MiB(32)
#define BOOT_RAM_BASE_ADDR	0xfff00000
#define BOOT_RAM_SIZE		KiB(64)
// IO
#define IO_BASE                 0x82000000

#define UART_BASE_ADDR		(IO_BASE + 0x00000)
#define UART_SIZE		0x200
#define KBD_UART_BASE_ADDR	(IO_BASE + 0x10000)
#define KBD_UART_SIZE		0x200
#define MSE_UART_BASE_ADDR	(IO_BASE + 0x20000)
#define MSE_UART_SIZE		0x200
#define AUX_UART_BASE_ADDR	(IO_BASE + 0x30000)
#define AUX_UART_SIZE		0x200
#define LCDC0_BASE_ADDR		(IO_BASE + 0x40000)
#define LCDC0_SIZE		0x200
#define LCDC1_BASE_ADDR		(IO_BASE + 0x50000)
#define LCDC1_SIZE		0x200
#define GPIO_BASE_ADDR		(IO_BASE + 0x60000)
#define GPIO_SIZE		0x200
#define INTC_BASE_ADDR		(IO_BASE + 0x70000)
#define INTC_SIZE               0x200
#define SPI0_BASE_ADDR		(IO_BASE + 0x80000) // SD storage (for now)
#define SPI0_SIZE               0x200
#define SPI1_BASE_ADDR          (IO_BASE + 0x90000) // WiFi/SDIO?
#define SPI1_SIZE               0x200
#define I2S_BASE_ADDR           (IO_BASE + 0xa0000)
#define I2S_SIZE                0x200
#define SDH_BASE_ADDR           (IO_BASE + 0xb0000)
#define SDH_SIZE                0x200
#define SBD0_BASE_ADDR		(IO_BASE + 0xf0000)
#define SBD_SIZE		0x00000020
#define SBD_NUM			4
#define SBD_STRIDE		0x100
// Other things:
// SD host, flash, ethernet.

#define UART0_IRQ	0
#define KBD_IRQ	        1
#define MSE_IRQ	        2
#define SD0_IRQ	        3
#define SBD0_IRQ	24
#define SBD_IRQ_STRIDE	1

/* Periodic callbacks: */
void platform_poll_lcdc0(u64 ticks);
#define LCDC0_POLL_PERIOD_MASK	0xfffff

static inline void	platform_poll_periodic(u64 ticks)
{
	if ((ticks & LCDC0_POLL_PERIOD_MASK) == 0)
		platform_poll_lcdc0(ticks);
	// FIXME:  LCDC1
}

void      platform_state_save(int handle);

#endif

#endif
