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

#ifndef DEVSBD_H
#define DEVSBD_H

#include "Device.h"
#include "log.h"
#include "AbstractIntc.h"
#include "Bus.h"


#define DEVSBD_REG_IDR		0
#define DEVSBD_REG_IDR_ID	0xfeedf00d

#define DEVSBD_REG_BLKS		1

#define DEVSBD_REG_IRQ		2

#define DEVSBD_REG_CMD		3
#define DEVSBD_REG_CMD_IDLE	0
#define DEVSBD_REG_CMD_RD	1
#define DEVSBD_REG_CMD_RD_PEND	2	// RD accepted, but will raise IRQ on completion
#define DEVSBD_REG_CMD_WR	3
#define DEVSBD_REG_CMD_WR_PEND  4	// WR accepted, but will raise IRQ on completion

#define DEVSBD_REG_PA		4

#define DEVSBD_REG_BLK_START   	5

#define DEVSBD_REG_LEN		6

#define DEVSBD_REG_INFO		7
#define DEVSBD_REG_INFO_BLKSZ_MASK	0xff	// In units of 512

#define DEVSBD_REG_END		DEVSBD_REG_INFO

#define DEVSBD_BLK_SIZE		512

class DevSBD : public Device {
public:
        DevSBD() : base_address(0), address_span(0), bus(0), intc(0), fd(-1), nr_blocks(0)
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
		DEBUG("DevSBD\n");
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

	void	openImage(char *filename);

private:
	void	init();
	void 	*getAddrDMA(PA addr);
	void	doRead(u32 block_start, u32 num, PA to);
	void	doWrite(u32 block_start, u32 num, PA from);

	// Types:
	PA	base_address;
	PA	address_span;

	Bus	*bus;
	AbstractIntc 	*intc;
	unsigned int	irq_number;

	int	fd;
	u32	nr_blocks;

	u32	regs[8];
};

#endif
