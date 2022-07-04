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

#ifndef DEVSPI_H
#define DEVSPI_H

#include "Device.h"
#include "AbstractSPIDev.h"
#include "log.h"
#include "types.h"



#define DEVSPI_REG_CTRL			0x00
#define DEVSPI_REG_CTRL_CPOL		0x00000001
#define DEVSPI_REG_CTRL_CPHA		0x00000002
#define DEVSPI_REG_CTRL_RUN_A		0x00000080
#define DEVSPI_REG_CTRL_XFLEN_A_SHIFT	8
#define DEVSPI_REG_CTRL_XFLEN_MASK      0x03	// Len is actually just 0-3 (1-4), until FIFO done
#define DEVSPI_REG_CTRL_RUN_B		0x00000400
#define DEVSPI_REG_CTRL_XFLEN_B_SHIFT	11

#define DEVSPI_REG_TXD_A		0x04

#define DEVSPI_REG_RXD_A		0x08

#define DEVSPI_REG_CS			0x0c
#define DEVSPI_REG_CS_CS0		0x01	// FIXME: Can do multiple CSes

#define DEVSPI_REG_CLKDIV		0x10
#define DEVSPI_REG_CLKDIV_DIV_MASK	0xff	// Divide by 1-256

#define DEVSPI_REG_TXD_B		0x14

#define DEVSPI_REG_RXD_B		0x18


class DevSPI : public Device {
public:
        DevSPI() : base_address(0), address_span(0),
		   config(0), len_a(0), len_b(0), cs(0),
                   txd_a(0), rxd_a(0), txd_b(0), rxd_b(0),
                   clkdiv(0), spidev(0)
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
		DEBUG("DevSPI\n");
	}

	void	addDevice(AbstractSPIDev *d);

private:
	void	init();

	uint32_t	doTransfer(uint32_t txd, uint32_t len);

	PA		base_address;
	PA		address_span;

	uint32_t	config;
        uint32_t        len_a, len_b;
	uint32_t	cs;
	uint32_t	txd_a, rxd_a, txd_b, rxd_b;
	uint8_t		clkdiv;
	AbstractSPIDev	*spidev;
};

#endif
