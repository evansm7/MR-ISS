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

#include "DevSimpleUart.h"
#include <unistd.h>
#include <poll.h>


static	void async_cb(void *arg)
{
	((DevSimpleUart *)arg)->asyncRxCB();
}

void 	DevSimpleUart::init(void)
{
	regs[UART_REG_IRQ_ENABLE/4] = 0;
	regs[UART_REG_IRQ_STATUS/4] = 0;
	regs[UART_REG_STATUS/4] = UART_REG_STATUS_TXNF;

	pthread_mutex_init(&mutex, NULL);

	if (serio) {
		serio->init();
		serio->registerAsyncRxCB(async_cb, this);
	} else {
		LOG("DevSimpleUart: No serio registered\n");
	}
}

void	DevSimpleUart::sendByte(u8 data)
{
	write(1, &data, 1);
	if (serio)
		serio->putByte(data);
}

u8	DevSimpleUart::getByte()
{
	u8 b;
	if (serio) {
		b = serio->getByte();
	} else {
		b = 0;
	}
	return b;
}

/* This is called when the backend thread notices input.  It isn't called again until getByte() is called. */
void	DevSimpleUart::asyncRxCB()
{
	pthread_mutex_lock(&mutex);
	regs[UART_REG_STATUS/4] |= UART_REG_STATUS_RXNE;
	regs[UART_REG_IRQ_STATUS/4] |= UART_REG_IRQ_STATUS_RX;
	assessIRQstatus();
	pthread_mutex_unlock(&mutex);
}

/* This might be called from main/CPU thread or from RX async notification thread.
 * Call this with mutex held.
 */
void	DevSimpleUart::assessIRQstatus()
{
	bool want_irq = false;

	/* Currently only RX data is supported; TX non-full and eoverflow are not
	 * emulated.
	 */

	if (regs[UART_REG_IRQ_STATUS/4] & regs[UART_REG_IRQ_ENABLE/4])
		want_irq = true;

	if (!irq_active && want_irq) {
		IOTRACE("DevSimpleUART: IRQ went active (IRQ_STATUS %02x), activating event %d on %p\n",
			regs[UART_REG_IRQ_STATUS/4], irq_number, intc);
		irq_active = true;
		raiseIRQ();
	} else if (irq_active && !want_irq) {
		irq_active = false;
		/* There's no lowerIRQ() since we deal with edges/events. */
	}
}

u32	DevSimpleUart::read32(PA addr)
{
	FATAL("DevSimpleUart::read32(%x)\n", addr);
	return 0;
}

u16	DevSimpleUart::read16(PA addr)
{
	FATAL("DevSimpleUart::read16(%x)\n", addr);
	return 0;
}

u8	DevSimpleUart::read8(PA addr)
{
	int reg = addr & 0xf;
	u8 data = 0;

	pthread_mutex_lock(&mutex);
	switch(reg) {
		case UART_REG_RX:
			data = getByte();
			regs[UART_REG_STATUS/4] &= ~UART_REG_STATUS_RXNE;
				/* Though there might be something else pending,
				 * the rx thread will trigger the async CB
				 * anyway.  Previously this checked
				 * serio->rxPending(), but isn't necessary.
				 */
			break;
		case UART_REG_STATUS:
		case UART_REG_IRQ_STATUS:
		case UART_REG_IRQ_ENABLE:
			data = regs[reg/4];
			break;

		default:
			FATAL("DevSimpleUart::read8(%x)\n", addr);
	}
	IOTRACE("DevSimpleUart: RD[%x] <= 0x%02x\n", addr, data);

	/* This does a couple of things; it updates the state of a
	 * level-sensitive IRQ (which we don't have) on RD, and updates the
	 * IIR:
	 */
	assessIRQstatus();
	pthread_mutex_unlock(&mutex);

	return data;
}

void	DevSimpleUart::write32(PA addr, u32 data)
{
	FATAL("DevSimpleUart::write32(%x, %x)\n", addr, data);
}

void	DevSimpleUart::write16(PA addr, u16 data)
{
	FATAL("DevSimpleUart::write16(%x, %x)\n", addr, data);
}

void	DevSimpleUart::write8(PA addr, u8 data)
{
	int reg = addr & 0xf;
	IOTRACE("DevSimpleUart: 0x%02x => WR[%x]\n", data, addr);

	pthread_mutex_lock(&mutex);

	switch(reg) {
		case UART_REG_TX:
			// FIXME:
			// Emulate TX FIFO; it needs to be able to fill up and
			// needs to then flag an IRQ on *change* from full to
			// non-full.
			sendByte(data);
			break;
		case UART_REG_IRQ_STATUS:
			regs[reg/4] &= ~data; // W1C
			break;
		case UART_REG_IRQ_ENABLE:
			regs[reg/4] = data;
			break;

		default:
			FATAL("DevSimpleUart::write8(%x, %x)\n", addr, data);
	}

	assessIRQstatus();
	pthread_mutex_unlock(&mutex);
}

