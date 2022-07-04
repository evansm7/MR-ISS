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

#ifndef UART_H
#define UART_H

#include <inttypes.h>
#include "hw.h"


void    uart_init(void);
void	uart_putch(char c);
char 	uart_getch(void);
int 	uart_ch_rdy(void);
void	uart_puts(char *str);


/*
 * Registers are byte-wide:
 *
 * 0x0: DATA_FIFO
 * 	RD:	      	RX data FIFO or UNKNOWN if FIFO empty.
 * 	WR:		TX data into FIFO, discarded if FIFO full.
 */
#define	UART_RX		0
#define	UART_TX		0

/* 0x4:	FIFO_STATUS
 * 	RD:	b0:	RX fifo non-empty (can read)
 * 		b1:	TX fifo non-full (can write)
 * 		b[7:2]	RAZ
 * 	WR:	<writes ignored>
 */
#define UART_STATUS	4
#define UART_STATUS_CAN_RX	B(0)
#define UART_STATUS_CAN_TX	B(1)

/* 0x8: IRQ_STATUS
 * 	RD:	b0:	IRQ0: RX fifo went non-empty
 * 		b1:	IRQ1: TX fifo went non-full
 * 		b2:	IRQ2: RX OVF occurred
 * 	WR:	W1C of status above
 */
#define UART_IRQ_STATUS	8
#define UART_IRQ_RX_FIFO_NE	B(0)
#define UART_IRQ_TX_FIFO_NF	B(1)
#define UART_IRQ_RX_FIFO_OVF	B(2)

/* 0xc:	IRQ_ENABLE
 * 	RD/WR:	b0:	Assert output for IRQ 0
 * 		b1:	'' IRQ 1
 * 		b2:	'' IRQ 2
 */
#define UART_IRQ_ENABLE 12

#endif
