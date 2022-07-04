/* Dumb but lovely bare-metal demo for MR-ISS
 *
 * 4/7/22 ME
 *
 * Copyright 2016-2022 Matt Evans
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


/* LCDC registers */
#define LCDC0_BASE_ADDR	LCDC0_PHYS_ADDR
#define REG_CTRL	0
#define REG_DIMS	1
#define REG_VS		2
#define REG_HS		3
#define REG_FB_ADDR	4
#define REG_ID		5
#define REG_DWPL	6
#define REG_PAL_IDX	7
#define REG_PAL		8
/* These regs are *little endian!* */
#define BS32(x)		( (((x) >> 24) & 0x000000ff) | \
			  (((x) >> 8)  & 0x0000ff00) | \
			  (((x) << 8)  & 0x00ff0000) | \
			  (((x) << 24) & 0xff000000) )

/* Simple 4BPP 320*240 framebuffer, demonstrating indexed modes
 * and upscaling:
 */
#define WIDTH 	320
#define HEIGHT 	240

#define FB_LEN (WIDTH*HEIGHT/2)
static uint32_t *framebuffer;

extern void draw_plasma(uint32_t *fb);

static void set_up_lcdc(void *fb)
{
	/* Set up LCDC:
	 * Note for MR-ISS the timing is irrelevant; but, this code also works
	 * on hardware where it matters.
	 */
	volatile uint32_t *lcdc = (volatile uint32_t *)LCDC0_BASE_ADDR;

	// Off
	lcdc[REG_CTRL] = 0;
	// Display 320x240 with mult x/y x2 making 640x480:
	lcdc[REG_DIMS] = BS32(640 | (1<<11) | (480 << 16) | (1 << 27));
	// Vsync width 2, fp 13, bp 34, invert polarity:
	lcdc[REG_VS] = BS32(2 | (13 << 8) | (34 << 16) | (1 << 24));
	// Hsync width 96, fp 15, bp 49, invert polarity of HS, invert DE, invert BLANK:
	lcdc[REG_HS] = BS32(96 | (15 << 8) | (49 << 16) | (1 << 24) | (1 << 25) | (1 << 26));
	// FIXME: Configure blank & DE polarity!
	lcdc[REG_FB_ADDR] = BS32((uintptr_t)fb);
	// DWords per line = 20 (320 pixels of 4bpp = 160 bytes), BPP 4 (log2 2)
	lcdc[REG_DWPL] = BS32(((320*4/8/8)-1) | (2 << 11));
	// Finally, program a palette mapping colours 0..15 to RGB
	for (int i = 0; i < 16; i++) {
		lcdc[REG_PAL_IDX] = BS32(i);
		lcdc[REG_PAL] = BS32(((i/2) << 12) | (i << 4));
	}
	// Finally, turn on the display!
	lcdc[REG_CTRL] = BS32((1 << 31));
}

int main(void)
{
	int i = 0;

	// Framebuffer must start on 8-byte boundary, so round up:
	framebuffer = (uint32_t *)FB_BASE;

	set_up_lcdc(framebuffer);

	while (1) {
		draw_plasma(framebuffer);

		if ((++i & 0x3f) == 0)
			uart_puts("Hello from PowerPC!\n");
	}

	return 0;
}
