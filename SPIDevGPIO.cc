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

/* SPIDevGPIO
 *
 * This calss provides an SPI backend that attaches to a physical device via
 * GPIO.  Not only is this cool, but it saves me tbe intense bother of writing
 * simulated SD cards, etc.
 *
 * 12 Apr 2021
 */

#define PI_GPIO		// Other interfaces might exist sometime

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "SPIDevGPIO.h"
#ifdef PI_GPIO
#include <pigpio.h>
#endif


////////////////////////////////////////////////////////////////////////////////
// IO definitions

#define GPIO_DOUT	7
#define GPIO_DIN	8
#define GPIO_CLK	9
#define GPIO_CS		10

////////////////////////////////////////////////////////////////////////////////

#ifdef PI_GPIO
static void gpioSighand(int sig)
{
	fprintf(stderr, "*** Signal %d\n", sig);
	if (sig == SIGINT) {
		fprintf(stderr, "*** Cleaning up GPIO config ***\n");
		gpioSetMode(GPIO_DOUT, PI_INPUT);
		gpioSetMode(GPIO_DIN, PI_INPUT);
		gpioSetMode(GPIO_CLK, PI_INPUT);
		gpioSetMode(GPIO_CS, PI_INPUT);
		gpioTerminate();

		exit(0);
	}
}
#else
#error "SPIDevGPIO: No matching interface implementation"
#endif

void 	SPIDevGPIO::init()
{
#ifdef PI_GPIO
	gpioCfgInterfaces(PI_DISABLE_FIFO_IF | PI_DISABLE_SOCK_IF);
	if (gpioInitialise() < 0) {
                fprintf(stderr, "pigpio init fail\n");
                exit(1);
        }

	gpioSetMode(GPIO_DOUT, PI_OUTPUT);
	gpioSetMode(GPIO_DIN, PI_INPUT);
	gpioSetMode(GPIO_CLK, PI_OUTPUT);
	gpioSetMode(GPIO_CS, PI_OUTPUT);

	gpioSetSignalFunc(SIGINT, gpioSighand);
#endif

#if 0
	// Test:
	while (1) {
		for (int i = 0; i < 8; i++) {
			gpioWrite(GPIO_DOUT, !!(i & 1));
			gpioWrite(GPIO_CLK, !!(i & 2));
			gpioWrite(GPIO_CS, !!(i & 4));
			printf("%1d ", gpioRead(GPIO_DIN));
			usleep(10000);
		}
		printf("\n");
	}
#endif
}

static inline void setClk(bool on, bool cpol)
{
#ifdef PI_GPIO
	gpioWrite(GPIO_CLK, (u8)on ^ (u8)cpol);
#endif
}

void  	SPIDevGPIO::setConfig(bool cpol, bool cpha, u8 clkdiv)
{
	this->cpol = cpol;
	this->cpha = cpha;
	this->clkdiv = clkdiv;

	// Set clock quiescent (0 for CPOL 0, 1 for CPOL 1)
	setClk(0, cpol);
}

u8      SPIDevGPIO::getPutByte(u8 b)
{
	u8 r = 0;
	// Clock stuff out, according to CPOL/CPHA, MSB first

	int clk = cpha; // Initial clock is 0 for phase 0, else 1
	setClk(clk, cpol);

	for (int bit = 7; bit >= 0; bit--) {
#ifdef PI_GPIO
		// Set up new data first
		gpioWrite(GPIO_DOUT, !!(b & (1 << bit)));
#endif
		// Then sample DIN, change clock:
#ifdef PI_GPIO
		r = (r << 1) | gpioRead(GPIO_DIN);
#endif
		clk = !clk;
		setClk(clk, cpol);

		// Finally, set up new clock (unless it's the last cycle & CPHA=1)
		if (!(cpha && bit == 0)) {
			clk = !clk;
			setClk(clk, cpol);
		}
	}
	setClk(0, cpol);
	return r;
}

void	SPIDevGPIO::setCS(bool v)
{
#ifdef PI_GPIO
	gpioWrite(GPIO_CS, v);
#endif
}
