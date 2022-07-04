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

#include "DevUart.h"
#include <unistd.h>
#include <poll.h>


static	void async_cb(void *arg)
{
	((DevUart *)arg)->asyncRxCB();
}

void 	DevUart::init(void)
{
	regs[UART_REG_RBR] = 0;
	regs[UART_REG_IER] = 0;
	regs[UART_REG_FCR] = 0;
	regs[UART_REG_LCR] = 0;
	regs[UART_REG_MCR] = 0; // Bit 0x80 = scale/4
	regs[UART_REG_LSR] = UART_REG_LSR_DEFAULT;
	regs[UART_REG_MSR] = 0;
	regs[UART_REG_SCR] = 0;

	regs[8 + UART_REG_IIR] = 0x01; // IRQ not pending
	regs[8 + UART_REG_DLL] = 0x45;
	regs[8 + UART_REG_DLM] = 0x01;

	pthread_mutex_init(&mutex, NULL);

	if (serio) {
		serio->init();
		serio->registerAsyncRxCB(async_cb, this);
	} else {
		LOG("DevUart: No serio registered\n");
	}
}

void	DevUart::sendByte(u8 data)
{
	// ToDo:  Something more elaborate with ptys...
	write(1, &data, 1);
	if (serio)
		serio->putByte(data);
}

u8	DevUart::getByte()
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
void	DevUart::asyncRxCB()
{
	pthread_mutex_lock(&mutex);
	regs[UART_REG_LSR] |= UART_REG_LSR_DR;
	assessIRQstatus();
	pthread_mutex_unlock(&mutex);
}

/* This might be called from main/CPU thread or from RX async notification thread.
 * Call this with mutex held.
 */
void	DevUart::assessIRQstatus()
{
	bool want_irq = false;

	/* Currently only data ready (DR) and TX empty (THRE) IRQs are
	 * supported; the line status events (e.g. break and overrun) aren't
	 * emulated.
	 */
	bool dr_irq = (regs[UART_REG_LSR] & UART_REG_LSR_DR) &&
		(regs[UART_REG_IER] & UART_REG_IER_RXDA);
	bool thre_irq = (regs[UART_REG_LSR] & UART_REG_LSR_THRE) &&
		(regs[UART_REG_IER] & UART_REG_IER_THRE);

	thre_irq = thre_irq & !thre_reported;

	want_irq = dr_irq || thre_irq;

	if (dr_irq)
		regs[8+UART_REG_IIR] = 4;
	else if (thre_irq)
		regs[8+UART_REG_IIR] = 2;
	else
		regs[8+UART_REG_IIR] = 1; /* None */

	if (!irq_active && want_irq) {
		IOTRACE("DevUART: IRQ went active (IIR %02x), activating event %d on %p\n",
			regs[8+UART_REG_IIR], irq_number, intc);
		irq_active = true;
		raiseIRQ();
		if (thre_irq && !thre_reported)
			thre_reported = true;
	} else if (irq_active && !want_irq) {
		irq_active = false;
		/* There's no lowerIRQ() since we deal with edges/events. */
	}
}

u32	DevUart::read32(PA addr)
{
	FATAL("DevUart::read32(%x)\n", addr);
	return 0;
}

u16	DevUart::read16(PA addr)
{
	FATAL("DevUart::read16(%x)\n", addr);
	return 0;
}

u8	DevUart::read8(PA addr)
{
	int reg = addr & 0x7;
	u8 data = 0;

	pthread_mutex_lock(&mutex);
	switch(reg) {
		case UART_REG_THR: /* Or DLL if page 1 */
			if (isPage1()) {
				data = regs[8+reg];
			} else {
				data = getByte();
				regs[UART_REG_LSR] &= ~UART_REG_LSR_DR;
				/* Though there might be something else pending,
				 * the rx thread will trigger the async CB
				 * anyway.  Previously this checked
				 * serio->rxPending(), but isn't necessary.
				 */
			}
			break;
		case UART_REG_IER: /* Or DLM if page1 */
			data = isPage1() ? regs[8+reg] : regs[reg];
			break;
		case UART_REG_IIR:
			data = regs[8+reg];	/* Hack, pretend this is on page 1 */
			break;
		case UART_REG_LCR:
			data = regs[reg];
			break;
		case UART_REG_MCR:
			data = regs[reg];
			break;
		case UART_REG_LSR:
			data = regs[reg]; // Get RX status for DR
			break;
		case UART_REG_MSR:
			data = regs[reg];
			break;
		case UART_REG_SCR:
			data = regs[reg];
			break;

		default:
			FATAL("DevUart::read8(%x)\n", addr);
	}
	IOTRACE("DevUART: RD[%x] <= 0x%02x\n", addr, data);

	/* This does a couple of things; it updates the state of a
	 * level-sensitive IRQ (which we don't have) on RD, and updates the
	 * IIR:
	 */
	assessIRQstatus();
	pthread_mutex_unlock(&mutex);

	return data;
}

void	DevUart::write32(PA addr, u32 data)
{
	FATAL("DevUart::write32(%x, %x)\n", addr, data);
}

void	DevUart::write16(PA addr, u16 data)
{
	FATAL("DevUart::write16(%x, %x)\n", addr, data);
}

void	DevUart::write8(PA addr, u8 data)
{
	int reg = addr & 0x7;
	IOTRACE("DevUART: 0x%02x => WR[%x]\n", data, addr);

	pthread_mutex_lock(&mutex);
	bool sent_byte = false;
	switch(reg) {
		case UART_REG_THR:
			if (isPage1()) {
				regs[8+reg] = data;
			} else {
				sendByte(data);
				sent_byte = true;
			}
			break;
		case UART_REG_IER:
			if (isPage1()) {
				regs[8+reg] = data;
			} else {
				regs[reg] = data;
			}
			break;
		case UART_REG_FCR: /* Same addr as IIR, but you can't write that. */
			regs[reg] = data;
			break;
		case UART_REG_LCR:
			regs[reg] = data;
			break;
		case UART_REG_MCR:
			regs[reg] = data;
			break;
		case UART_REG_LSR:
			regs[reg] = data;
			break;
		case UART_REG_MSR:
			regs[reg] = data;
			break;
		case UART_REG_SCR:
			regs[reg] = data;
			break;

		default:
			FATAL("DevUart::write8(%x, %x)\n", addr, data);
	}

	/* Technically, THRE goes 0 then 1 now, in zero time. */
	if (sent_byte)
		thre_reported = false;
	assessIRQstatus();
	pthread_mutex_unlock(&mutex);
}

