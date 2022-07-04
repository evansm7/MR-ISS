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

/* DevSPI
 *
 * Emulation of the MattRISC platform SPI host
 *
 * This object interfaces to an SPIDev instance,
 *
 * ME 11 April 2021
 */

#include <stdio.h>
#include "DevSPI.h"

//#define SPI_DEBUG

////////////////////////////////////////////////////////////////////////////////
// Register interface

u32	DevSPI::read32(PA addr)
{
	u32 data = 0;
	int reg = addr & 0xfc;

	switch (reg) {
		case DEVSPI_REG_CTRL:
			data = config | (len_a << DEVSPI_REG_CTRL_XFLEN_A_SHIFT) |
                                (len_b << DEVSPI_REG_CTRL_XFLEN_B_SHIFT);
			// FIXME: RUN reads as zero -- need to emulate "takes time" properly
			break;

		case DEVSPI_REG_TXD_A:
			data = txd_a;
			break;

		case DEVSPI_REG_RXD_A:
			data = rxd_a;
			break;

                case DEVSPI_REG_TXD_B:
                        data = txd_b;
			break;

		case DEVSPI_REG_RXD_B:
			data = rxd_b;
			break;

		case DEVSPI_REG_CS:
			data = cs;
			break;

		case DEVSPI_REG_CLKDIV:
			data = clkdiv;
			break;

		default:
			FATAL("DevSPI: Read from undef reg, addr 0x%08x (reg %d)\n", addr, reg);
	}

	IOTRACE("DevSPI: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

u16	DevSPI::read16(PA addr)
{
	FATAL("DevSPI::read16(%x)\n", addr);
	return 0;
}

u8	DevSPI::read8(PA addr)
{
	FATAL("DevSPI::read8(%x)\n", addr);
	return 0;
}

uint32_t	DevSPI::doTransfer(uint32_t txd, uint32_t len)
{
	/* This doesn't correctly emulate that a transfer takes some time;
	 * the transfer occurs here and any read of the regs sees the transfer
	 * occur in zero time.  Ideally software would see the RUN bit stay 1 for
	 * a while (to ensure the polling/completion code paths work).
	 *
	 * Ideas for this:
	 * - Expect that there'll be at least one read of CTRL to poll RUN; here
	 *   mark the transfer pending, and do the transfer after one read of CTRL
	 * - Transfer one byte per read of CTRL
	 */

	if (!spidev)
		return 0;

	uint32_t r = 0;

#ifdef SPI_DEBUG
	printf("[SPI transfer: ");
#endif
	for (unsigned int i = 0; i < len+1; i++) {
		uint8_t t = (txd >> (i*8)) & 0xff; 	// TXD reg little-endian!
		uint8_t b = spidev->getPutByte(t);
		r |= (u32)b << (i*8); // RXD little-endian!
#ifdef SPI_DEBUG
		printf("%02x/%02x ", t, b);
#endif
	}
#ifdef SPI_DEBUG
	printf("]\n");
#endif

	return r;
}

void	DevSPI::write32(PA addr, u32 data)
{
	int reg = addr & 0xfc;

	IOTRACE("DevSPI: WR[%x] => 0x%08x\n", addr, data);

	switch (reg) {
		case DEVSPI_REG_CTRL: {
			config = data & (DEVSPI_REG_CTRL_CPOL | DEVSPI_REG_CTRL_CPHA);
			len_a = (data >> DEVSPI_REG_CTRL_XFLEN_A_SHIFT) & DEVSPI_REG_CTRL_XFLEN_MASK;
			len_b = (data >> DEVSPI_REG_CTRL_XFLEN_B_SHIFT) & DEVSPI_REG_CTRL_XFLEN_MASK;

			if (spidev) {
				spidev->setConfig(!!(config & DEVSPI_REG_CTRL_CPOL),
						  !!(config & DEVSPI_REG_CTRL_CPHA),
						  clkdiv);
			}

			if (data & DEVSPI_REG_CTRL_RUN_A)
                                rxd_a = doTransfer(txd_a, len_a);
			if (data & DEVSPI_REG_CTRL_RUN_B)
                                rxd_b = doTransfer(txd_b, len_b);
		} break;

		case DEVSPI_REG_TXD_A:
			txd_a = data;
			break;

		case DEVSPI_REG_RXD_A:
			break;

                case DEVSPI_REG_TXD_B:
			txd_b = data;
			break;

		case DEVSPI_REG_RXD_B:
			break;

		case DEVSPI_REG_CS:
			cs = data;
			if (spidev)
				spidev->setCS(cs & DEVSPI_REG_CS_CS0);

			break;

		case DEVSPI_REG_CLKDIV:
			clkdiv = data & DEVSPI_REG_CLKDIV_DIV_MASK;
			if (spidev) {
				spidev->setConfig(!!(config & DEVSPI_REG_CTRL_CPOL),
						  !!(config & DEVSPI_REG_CTRL_CPHA),
						  clkdiv);
			}
			break;

		default:
			FATAL("DevSPI: Write of 0x%08x to undef reg, addr 0x%08x\n", data, addr);
	}
}

void	DevSPI::write16(PA addr, u16 data)
{
	FATAL("DevSPI::write16(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSPI::write8(PA addr, u8 data)
{
	FATAL("DevSPI::write8(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSPI::addDevice(AbstractSPIDev *d)
{
	spidev = d;
	spidev->init();
}

void	DevSPI::init()
{
}
