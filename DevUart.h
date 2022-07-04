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

/* A very very very simple just-enough 16550 UART.
 */

#ifndef DEVUART_H
#define DEVUART_H

#include <pthread.h>

#include "Device.h"
#include "log.h"
#include "AbstractIntc.h"
#include "AbstractSerial.h"

// Page 0:
#define UART_REG_RBR  0
#define UART_REG_THR  UART_REG_RBR
#define UART_REG_IER  1
#define UART_REG_IER_RXDA	0x01
#define UART_REG_IER_THRE	0x02
#define UART_REG_IER_RXLS	0x04
#define UART_REG_IER_MODEM	0x08
/* A bit of a hack, FCR on write and IIR on read but stored in 2 separate bytes: */
#define UART_REG_FCR  2
#define UART_REG_LCR  3
#define UART_REG_LCR_DLAB	0x80	/* For page 1 */
#define UART_REG_MCR  4
#define UART_REG_LSR  5
#define UART_REG_LSR_DR		0x01
#define UART_REG_LSR_OE		0x02
#define UART_REG_LSR_PE		0x04
#define UART_REG_LSR_FE		0x08
#define UART_REG_LSR_BI		0x10
#define UART_REG_LSR_THRE 	0x20
#define UART_REG_LSR_TEMT 	0x40
#define UART_REG_LSR_ERR  	0x80
#define UART_REG_LSR_DEFAULT	(UART_REG_LSR_THRE | UART_REG_LSR_TEMT)
#define UART_REG_MSR  6
#define UART_REG_SCR  7
// Page 1:
#define UART_REG_IIR  UART_REG_FCR
#define UART_REG_DLL  UART_REG_RBR
#define UART_REG_DLM  UART_REG_IER

class DevUart : public Device {
public:
        DevUart() :
	base_address(0), address_span(0), intc(0), irq_active(false), serio(0), thre_reported(false) {}

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
		DEBUG("DevUart\n");
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
	bool	isPage1()	{ return (regs[UART_REG_LCR] & UART_REG_LCR_DLAB); }

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

	u8	regs[16];

	AbstractIntc 	*intc;
	unsigned int	irq_number;
	bool	irq_active;
	AbstractSerial	*serio;

	bool	thre_reported;
	/* This mutex protects the interrupt state/regs. */
	pthread_mutex_t	mutex;
};

#endif
