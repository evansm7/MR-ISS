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

#ifndef DEVSD_H
#define DEVSD_H

#include "Device.h"
#include "log.h"
#include "AbstractIntc.h"
#include "Bus.h"

#include "DevSDCard.h"


#define DEVSD_REG_END			16


class DevSD : public Device {
public:
        DevSD() : base_address(0), address_span(0), bus(0),
                  intc(0), irq_number(0), sdcard(NULL)
	{}

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
		DEBUG("DevSD\n");
	}

	void	setIntc(AbstractIntc *i, unsigned int irq)
	{
		intc = i;
		irq_number = irq;
	}

	void	setBus(Bus *b)
	{
		bus = b;
	}

	// Platform provides periodic callbacks:
	void	periodic(u64 ticks);

        void	init(DevSDCard *sdc);

private:
	void 	*getAddrDMA(PA addr);
        bool    cmdPending();
        bool    txPending();
        bool    rxPending();

        void    checkForWork();

        uint32_t regs[DEVSD_REG_END];

	// Types:
	PA	base_address;
	PA	address_span;

	Bus		*bus;
	AbstractIntc 	*intc;
	unsigned int	irq_number;

        DevSDCard       *sdcard;

        bool            irq_tx_triggered;
        bool            irq_tx_enabled;
        bool            irq_rx_triggered;
        bool            irq_rx_enabled;

        unsigned int    txblocks;

        bool            cmd_ack;
        bool            tx_ack;
        bool            rx_ack;

        unsigned int    cmd_status;
        unsigned int    tx_status;
        unsigned int    rx_status;
        unsigned int    dma_status;
};

#endif
