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

/* A very very very simple implementation of Xilinx xps-intc IRQ controller
 */

#ifndef DEVXPSINTC_H
#define DEVXPSINTC_H

#include "Device.h"
#include "log.h"
#include "PPCCPUState.h"
#include "AbstractIntc.h"

#define XPSINTC_REG_ISR	0x0
#define XPSINTC_REG_IPR	0x4
#define XPSINTC_REG_IER	0x8
#define XPSINTC_REG_IAR	0xC
#define XPSINTC_REG_SIE	0x10
#define XPSINTC_REG_CIE	0x14
#define XPSINTC_REG_IVR	0x18
#define XPSINTC_REG_MER	0x1C

class DevXpsIntc : public Device, public AbstractIntc {
public:
	DevXpsIntc() : base_address(0), address_span(0), cpus(0)
	{
		init();
	}

	u32	read32(PA addr);
	u16	read16(PA addr);
	u8	read8(PA addr);

	void	write32(PA addr, u32 data);
	void	write16(PA addr, u16 data);
	void	write8(PA addr, u8 data);

	void	setProps(PA addr, PA size)
	{
		base_address = addr;
		address_span = size;
	}

	void	dump()
	{
		DEBUG("DevXpsIntc\n");
	}

	/* Own functions not in parent abstract class... */
	void	triggerIRQ(unsigned int n);	// The world is an edge

	/* Tell this controller about CPU, so it can have EI
	 * activated/deactivated:
	 */
	void	setCPUState(PPCCPUState *cs)	{ cpus = cs; }

private:
	void	init();

	// Types:
	PA	base_address;
	PA	address_span;

	u32	regs[16];

	bool	triggeredCPU;

	PPCCPUState *cpus;

	void	reassessOutput();
	u32	getActive();
};

#endif
