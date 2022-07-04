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

/* Implementing my dead-simple APB UART
 */

#ifndef DEVSIMPLEUART_H
#define DEVSIMPLEUART_H

#include <pthread.h>

#include "Device.h"
#include "log.h"
#include "AbstractIntc.h"
#include "AbstractSerial.h"

#define UART_REG_TX  		0x00
#define UART_REG_RX  		0x00

#define UART_REG_STATUS		0x04
#define UART_REG_STATUS_RXNE	1
#define UART_REG_STATUS_TXNF	2

#define UART_REG_IRQ_STATUS	0x08
#define UART_REG_IRQ_STATUS_RX	1
#define UART_REG_IRQ_STATUS_TX	2
#define UART_REG_IRQ_STATUS_OVF	4

#define UART_REG_IRQ_ENABLE	0x0c
#define UART_REG_IRQ_ENABLE_RX	1
#define UART_REG_IRQ_ENABLE_TX	2
#define UART_REG_IRQ_ENABLE_OVF	4

class DevSimpleUart : public Device {
public:
        DevSimpleUart() :
	base_address(0), address_span(0), intc(0), irq_active(false), serio(0) {}

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
		DEBUG("DevSimpleUart\n");
	}

	void	setIntc(AbstractIntc *i, unsigned int irq)
	{
		intc = i;
		irq_number = irq;
	}

	void	init();

	void	setSerIO(AbstractSerial *s)
	{
		serio = s;
	}

	/* Don't call this */
	void	asyncRxCB();

private:
	void	sendByte(u8 data);
	u8	getByte();

	void	assessIRQstatus();

	void	raiseIRQ()
	{
		if (intc)
			intc->triggerIRQ(irq_number);
	}

	// Types:
	PA	base_address;
	PA	address_span;

	u8	regs[4];

	AbstractIntc 	*intc;
	unsigned int	irq_number;
	bool	irq_active;
	AbstractSerial	*serio;

	/* This mutex protects the interrupt state/regs. */
	pthread_mutex_t	mutex;
};

#endif
