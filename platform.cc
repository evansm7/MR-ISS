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

/* MR-ISS platform setup
 *
 * This file sets up the simulation platform:
 * - Memory map
 * - Choice of & location of devices
 *
 * Currently supports these platforms:
 *
 * 	MR1:  Original s/w dev platform (Linux w/ 16550-alike, SBD, IRQC)
 *	MR2:  Intermediate HW platform with LCD
 *	MR3:  proper Linux/IO/LCD/SD HW platform
 */

#include "Config.h"
#include "PPCInterpreter.h"
#include "Device.h"
#include "Bus.h"
#include "DevUart.h"
#include "DevSimpleUart.h"
#include "DevDebug.h"
#include "DevRAM.h"
#include "DevXpsIntc.h"
#include "DevGPIO.h"
#include "DevDummy.h"
#include "SerialTCP.h"
#include "SerialPTY.h"
#include "DevSBD.h"
#include "DevSPI.h"
#include "SPIDevGPIO.h"
#include "platform.h"
#include "sim_state.h"
#include "DevSD.h"


#if PLATFORM == 1
// Devices
DevUart		devuart;
DevRAM		devram;
DevXpsIntc	devintc;
DevSBD		devsbd[SBD_NUM];
#endif

#if PLATFORM == 2
#include "DevLCDC.h"

// Devices
DevSimpleUart	devuart;
DevRAM		devram;
DevLCDC		devlcdc_pdp; // Drives PDP
#endif

#if PLATFORM == 3
#include "DevLCDC.h"

// Devices
DevRAM		devram;
DevRAM		devbootram;
DevSimpleUart	devuart;
DevSimpleUart   devkbd;
DevSimpleUart   devmse;
// aux UARTs
DevLCDC		devlcdc_pdp; // Drives PDP
DevGPIO		devgpio;
DevXpsIntc	devintc;
DevDummy        devi2s; // Dummy, permit probe
DevSPI		devspi0;
DevSPI		devspi1;

DevSBD		devsbd[SBD_NUM];
#ifdef GPIO
SPIDevGPIO	devspi_gpio;
#endif

DevSDCard       devsd_card;
DevSD           devsd;
#endif

/* Common */

/* Console goes to a pty; alternative of a TCP socket thus:
 * SerialTCP	serio(9999, false);
 */
SerialPTY	serio(false);


void	platform_init(Bus *bus)
{
	/* RAM (bank 0) init is common (all platforms have at least one) */
	devram.init(RAM_SIZE);

	// Attach to bus:
	bus->attach(&devram, RAM_BASE_ADDR, RAM_SIZE);

#if PLATFORM == 1
	/* Image is assumed to be loaded into bank 0.  Fine for MR. */
	if (CFG(load_addr) >= RAM_BASE_ADDR) {
		devram.loadAtOffset(CFG(rom_path), CFG(load_addr) - RAM_BASE_ADDR);
	} else {
		FATAL("Load address 0x%x is beneath RAM start, 0x%x\n",
		      CFG(load_addr), RAM_BASE_ADDR);
	}

	devuart.setSerIO(&serio);
	devuart.init();
	// Assign UART IRQ:
	devuart.setIntc(&devintc, UART0_IRQ);

        bus->attach(&devuart, UART_BASE_ADDR, UART_SIZE);

	bus->attach(&devintc, INTC_BASE_ADDR, INTC_SIZE);

	for (int i = 0; i < SBD_NUM; i++) {
		devsbd[i].setBus(bus);
		devsbd[i].setIntc(&devintc, SBD0_IRQ + (i * SBD_IRQ_STRIDE));
		bus->attach(&devsbd[i], SBD0_BASE_ADDR + (i * SBD_STRIDE), SBD_SIZE);

		if (CFG(block_path[i]))
			devsbd[i].openImage(CFG(block_path[i]));
	}
#endif

#if PLATFORM == 2
	if (CFG(load_addr) >= RAM_BASE_ADDR) {
		devram.loadAtOffset(CFG(rom_path), CFG(load_addr) - RAM_BASE_ADDR);
	} else {
		FATAL("Load address 0x%x is beneath RAM start, 0x%x\n",
		      CFG(load_addr), RAM_BASE_ADDR);
	}

	devuart.setSerIO(&serio);
	devuart.init();

        bus->attach(&devuart, UART_BASE_ADDR, UART_SIZE);

	// Add PDP-drivin' display controller:
	devlcdc_pdp.setStyle(DevLCDC::STYLE_PDP);
	devlcdc_pdp.setBus(bus);
	// devlcdc_pdp.setIntc(bus); // Not yet
	bus->attach(&devlcdc_pdp, LCDC0_BASE_ADDR, LCDC0_SIZE);
#endif

#if PLATFORM == 3
	devbootram.init(BOOT_RAM_SIZE);
	bus->attach(&devbootram, BOOT_RAM_BASE_ADDR, BOOT_RAM_SIZE);

	if (CFG(load_addr) >= BOOT_RAM_BASE_ADDR) {
		devbootram.loadAtOffset(CFG(rom_path), CFG(load_addr) - BOOT_RAM_BASE_ADDR);
	} else if (CFG(load_addr) >= RAM_BASE_ADDR) {
		devram.loadAtOffset(CFG(rom_path), CFG(load_addr) - RAM_BASE_ADDR);
		// ...which might fail.
	}

	devuart.setSerIO(&serio);
	devuart.init();
	// Assign UART IRQ:
	devuart.setIntc(&devintc, UART0_IRQ);
        bus->attach(&devuart, UART_BASE_ADDR, UART_SIZE);

        devkbd.init();
        devkbd.setIntc(&devintc, KBD_IRQ);
        bus->attach(&devkbd, KBD_UART_BASE_ADDR, KBD_UART_SIZE);
        devmse.init();
        devmse.setIntc(&devintc, MSE_IRQ);
        bus->attach(&devmse, MSE_UART_BASE_ADDR, MSE_UART_SIZE);

	bus->attach(&devintc, INTC_BASE_ADDR, INTC_SIZE);

	// Add PDP-drivin' display controller:
	devlcdc_pdp.setStyle(DevLCDC::STYLE_PDP);
	devlcdc_pdp.setBus(bus);
	// devlcdc_pdp.setIntc(bus); // Not yet
	bus->attach(&devlcdc_pdp, LCDC0_BASE_ADDR, LCDC0_SIZE);

	// GPIO:
	devgpio.setInputs(CFG(gpio_inputs));
	bus->attach(&devgpio, GPIO_BASE_ADDR, GPIO_SIZE);

        // I2S (dummy for now, to permit probe to fail gracefully):
        bus->attach(&devi2s, I2S_BASE_ADDR, I2S_SIZE);

        // SD host controller: uses first block filename
        devsd_card.init(CFG(block_path[0]));
        devsd.init(&devsd_card);
        devsd.setBus(bus);
        devsd.setIntc(&devintc, SD0_IRQ);
        bus->attach(&devsd, SDH_BASE_ADDR, SDH_SIZE);

	// For testing, it's handy to have SBDs.  They won't exist on real hardware though:
	for (int i = 0; i < SBD_NUM; i++) {
		devsbd[i].setBus(bus);
		devsbd[i].setIntc(&devintc, SBD0_IRQ + (i * SBD_IRQ_STRIDE));
		bus->attach(&devsbd[i], SBD0_BASE_ADDR + (i * SBD_STRIDE), SBD_SIZE);

		if (CFG(block_path[i]))
			devsbd[i].openImage(CFG(block_path[i]));
	}

	// SPI0
#ifdef GPIO
	// We attach to real hardware, yay!
	devspi0.addDevice(&devspi_gpio);
#endif
	bus->attach(&devspi0, SPI0_BASE_ADDR, SPI0_SIZE);
        bus->attach(&devspi1, SPI1_BASE_ADDR, SPI1_SIZE);
#endif
}

void 	platform_irq_cpuclient(PPCCPUState *pcs)
{
#if PLATFORM == 1 || PLATFORM == 3
	devintc.setCPUState(pcs);
#endif
}

/* Grotty hack for OS.cc grotty hacks */
unsigned int	platform_meminfo(meminfo_t info_out[])
{
        info_out[0].ram_pa_base = RAM_BASE_ADDR;
        info_out[0].ram_size = RAM_SIZE;
        info_out[0].host_map_base = devram.getMapBase();

        return 1;
}

#if PLATFORM == 2 || PLATFORM == 3
void platform_poll_lcdc0(u64 ticks)
{
	devlcdc_pdp.periodic(ticks);
}
#endif

#if PLATFORM == 3
void    platform_state_save(int handle)
{
	// INTC (can use regular read accessors to see all state)
        // FIXME: will need to do something smart about edge inputs; levels will sort themselves out.
        SAVE_REG_CHUNK(handle, devintc.read32(XPSINTC_REG_ISR), "IC_ISR");
        SAVE_REG_CHUNK(handle, devintc.read32(XPSINTC_REG_IER), "IC_IER");
        SAVE_REG_CHUNK(handle, devintc.read32(XPSINTC_REG_MER), "IC_MER");

	// Console UART
        // Really, very basic: we drop FIFO contents, just make sure IRQs work.
        SAVE_REG_CHUNK(handle, devuart.read8(UART_REG_STATUS), "CON_SR");
        SAVE_REG_CHUNK(handle, devuart.read8(UART_REG_IRQ_STATUS), "CON_ISR");
        SAVE_REG_CHUNK(handle, devuart.read8(UART_REG_IRQ_ENABLE), "CON_IER");

        // FIXME:
	// Other UARTs
	// I2S
	// LCDC0/1
        // GPIO

        return;
 err:
        WARN("Failed to save platform/device state!\n");
}
#endif
