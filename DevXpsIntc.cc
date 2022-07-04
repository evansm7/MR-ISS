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

#include "DevXpsIntc.h"
#include <unistd.h>


void 	DevXpsIntc::init(void)
{
	regs[XPSINTC_REG_ISR >> 2] = 0;
	regs[XPSINTC_REG_IPR >> 2] = 0;
	regs[XPSINTC_REG_IER >> 2] = 0;
	regs[XPSINTC_REG_IAR >> 2] = 0;
	regs[XPSINTC_REG_SIE >> 2] = 0;
	regs[XPSINTC_REG_CIE >> 2] = 0;
	regs[XPSINTC_REG_IVR >> 2] = ~0;
	regs[XPSINTC_REG_MER >> 2] = 0;

	triggeredCPU = false;
}

u8	DevXpsIntc::read8(PA addr)
{
	FATAL("DevXpsIntc::read8(%x)\n", addr);
	return 0;
}

u16	DevXpsIntc::read16(PA addr)
{
	FATAL("DevXpsIntc::read16(%x)\n", addr);
	return 0;
}

u32	DevXpsIntc::read32(PA addr)
{
	int reg = addr & 0x1c;
	u32 data = 0;

	switch(reg) {
		case XPSINTC_REG_ISR:
		case XPSINTC_REG_IER:
		case XPSINTC_REG_MER:
			data = regs[reg >> 2];
			break;

		case XPSINTC_REG_IAR:	/* Write-only locations */
		case XPSINTC_REG_SIE:
		case XPSINTC_REG_CIE:
			data = 0;
			break;

		case XPSINTC_REG_IPR: {
			u32 active = getActive();
			data = active;
		} break;

		case XPSINTC_REG_IVR: {
			u32 active = getActive();
			data = 0xffffffff;

			for (unsigned int i = 0; i < 32; i++) {
				if (active & (1 << i)) {
					data = i;
					break;
				}
			}
		} break;

		default:
			FATAL("DevXpsIntc::read8(%x)\n", addr);
	}
	IOTRACE("DevXpsIntc: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

void	DevXpsIntc::write8(PA addr, u8 data)
{
	FATAL("DevXpsIntc::write8(%x, %x)\n", addr, data);
}

void	DevXpsIntc::write16(PA addr, u16 data)
{
	FATAL("DevXpsIntc::write16(%x, %x)\n", addr, data);
}

void	DevXpsIntc::write32(PA addr, u32 data)
{
	int reg = addr & 0x1c;
	IOTRACE("DevXpsIntc: 0x%08x => WR[%x]\n", data, addr);

	switch(reg) {
		case XPSINTC_REG_ISR:
		case XPSINTC_REG_IER:
		case XPSINTC_REG_MER:
			regs[reg >> 2] = data;
			break;

		case XPSINTC_REG_IPR:	/* Read-only */
		case XPSINTC_REG_IVR:
			break;

		case XPSINTC_REG_IAR:	/* W1C */
			regs[XPSINTC_REG_ISR >> 2] &= ~data;
			break;

		case XPSINTC_REG_SIE:
			regs[XPSINTC_REG_IER >> 2] |= data;
			break;

		case XPSINTC_REG_CIE:
			regs[XPSINTC_REG_IER >> 2] &= ~data;
			break;

		default:
			FATAL("DevXpsIntc::write32(%x, %x)\n", addr, data);
	}
	reassessOutput();
}

u32	DevXpsIntc::getActive()
{
	u32 isr = regs[XPSINTC_REG_ISR >> 2];
	u32 ier = regs[XPSINTC_REG_IER >> 2];
	return isr & ier;
}

void	DevXpsIntc::reassessOutput()
{
	u32 active = getActive();

	if (active && !triggeredCPU) {
		triggeredCPU = true;
		IOTRACE("INTC: Triggering IRQ on CPU\n");
		if (cpus)
			cpus->assertIRQ();
	} else if (!active && triggeredCPU) {
		triggeredCPU = false;
		IOTRACE("INTC: De-asserting IRQ on CPU\n");
		if (cpus)
			cpus->assertIRQ(false);
	}
}

/* Doesn't have any support for levels yet... */
void	DevXpsIntc::triggerIRQ(unsigned int n)
{
	if (n < 32) {
		regs[XPSINTC_REG_ISR >> 2] |= 1 << n;
	}
	IOTRACE("INTC: IRQ %d triggered; ISR %08x, IER %08x\n", n,
		regs[XPSINTC_REG_ISR >> 2], regs[XPSINTC_REG_IER >> 2]);
	reassessOutput();
}
