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

#include "DevGPIO.h"


u32	DevGPIO::read32(PA addr)
{
	u32 data = 0;

	switch(addr & 0xf) {
		case GPIO_REG_OUT:
		case GPIO_REG_OUT_W1S:
		case GPIO_REG_OUT_W1C:
			data = out_state;
			break;
		case GPIO_REG_IN:
			data = in_state;
			break;

		default:
			FATAL("DevGPIO::read32(%x)\n", addr);
	}
	IOTRACE("DevGPIO: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

u16	DevGPIO::read16(PA addr)
{
	FATAL("DevGPIO::read16(%x)\n", addr);
	return 0;
}

u8	DevGPIO::read8(PA addr)
{
	FATAL("DevGPIO::read8(%x)\n", addr);
	return 0;
}

void	DevGPIO::write32(PA addr, u32 data)
{
	IOTRACE("DevGPIO: 0x%08x => WR[%x]\n", data, addr);

	u32 old_out = out_state;

	switch(addr & 0xf) {
		case GPIO_REG_OUT:
			out_state = data;
			break;
		case GPIO_REG_OUT_W1S:
			out_state |= data;
			break;
		case GPIO_REG_OUT_W1C:
			out_state &= ~data;
			break;
		case GPIO_REG_IN:
			// NOP
			break;

		default:
			FATAL("DevGPIO::write32(%x, %x)\n", addr, data);
	}

	if (old_out ^ out_state) {
		DEBUG("DevGPIO: Output bits %08x\n", out_state);
	}
}

void	DevGPIO::write16(PA addr, u16 data)
{
	FATAL("DevGPIO::write16(%x, %x)\n", addr, data);
}

void	DevGPIO::write8(PA addr, u8 data)
{
	FATAL("DevGPIO::write8(%x, %x)\n", addr, data);
}

