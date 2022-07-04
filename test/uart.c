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

#include <inttypes.h>
#include "uart.h"


static	volatile uint8_t	*uart_regs = (volatile uint8_t *)UART_PHYS_ADDR;

void    uart_init(void)
{
}

void	uart_putch(char c)
{
	while (!(uart_regs[UART_STATUS] & UART_STATUS_CAN_TX)) {};
	uart_regs[UART_TX] = c;
}

char 	uart_getch(void)
{
        while (!uart_ch_rdy()) {};
	return uart_regs[UART_RX];
}

int 	uart_ch_rdy(void)
{
	return !!(uart_regs[UART_STATUS] & UART_STATUS_CAN_RX);
}

void	uart_puts(char *str)
{
	while (*str) {
		char c = *str;
		if (c == '\n')
			uart_putch('\r');
		uart_putch(c);
		str++;
	}
}
