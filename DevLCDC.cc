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

/* DevLCDC
 *
 * Emulation of the MattRISC platform's LCDC.
 *
 * First go at PDP vs regular output -- FIXME here, though.
 * Also, does not support changing resolution (or any non-VGA).
 *
 * ME 4 June 2020
 */

#include <stdio.h>
#ifndef NO_SDL
#include <SDL.h>
#include "portable_endian.h"
#endif
#include "DevLCDC.h"



#define DISP_WIDTH 640
#define DISP_HEIGHT 480
#define DISP_SCALE 1


////////////////////////////////////////////////////////////////////////////////
// Register interface

u32	DevLCDC::read32(PA addr)
{
	u32 data = 0;
	int reg = (addr & 0x3f) >> 2;

	switch (reg) {
		case DEVLCDC_REG_STAT_CTRL:
		case DEVLCDC_REG_DSIZE:
		case DEVLCDC_REG_VSYNC:
		case DEVLCDC_REG_HSYNC:
		case DEVLCDC_REG_FB_BASE:
		case DEVLCDC_REG_DWPL_BPP:
		case DEVLCDC_REG_PAL_IDX:
			data = regs[reg];
			break;
		case DEVLCDC_REG_ID:
			data = DEVLCDC_REG_ID_VAL;
			break;
		case DEVLCDC_REG_PAL_ENTRY:
			data = palette[regs[DEVLCDC_REG_PAL_IDX] & DEVLCDC_REG_PAL_IDX_MASK];
			break;

		default:
			FATAL("DevLCDC: Read from undef reg, addr 0x%08x (reg %d)\n", addr, reg);
	}

	IOTRACE("DevLCDC: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

u16	DevLCDC::read16(PA addr)
{
	FATAL("DevLCDC::read16(%x)\n", addr);
	return 0;
}

u8	DevLCDC::read8(PA addr)
{
	FATAL("DevLCDC::read8(%x)\n", addr);
	return 0;
}

void	DevLCDC::write32(PA addr, u32 data)
{
	int reg = (addr & 0x3f) >> 2;

	IOTRACE("DevLCDC: WR[%x] => 0x%08x\n", addr, data);

	switch (reg) {
		case DEVLCDC_REG_STAT_CTRL:
			regs[reg] = data & (DEVLCDC_REG_STAT_CTRL_BEN |
					    DEVLCDC_REG_STAT_CTRL_EN);
			display_enable = !!(data & DEVLCDC_REG_STAT_CTRL_EN);
			border_enable = !!(data & DEVLCDC_REG_STAT_CTRL_BEN);
			DEBUG("DevLCDC: Border enable %d, display enable %d\n",
			      border_enable, display_enable);
			break;
		case DEVLCDC_REG_DSIZE:
			regs[reg] = data & (DEVLCDC_REG_DSIZE_WIDTH_MASK |
					    DEVLCDC_REG_DSIZE_XMUL_MASK |
					    DEVLCDC_REG_DSIZE_HEIGHT_MASK |
					    DEVLCDC_REG_DSIZE_YMUL_MASK);
			display_x = (data & DEVLCDC_REG_DSIZE_WIDTH_MASK) >>
				DEVLCDC_REG_DSIZE_WIDTH_SHIFT;
			display_y = (data & DEVLCDC_REG_DSIZE_HEIGHT_MASK) >>
				DEVLCDC_REG_DSIZE_HEIGHT_SHIFT;
			scale_x = ((data & DEVLCDC_REG_DSIZE_XMUL_MASK) >>
				   DEVLCDC_REG_DSIZE_XMUL_SHIFT) + 1;
			scale_y = ((data & DEVLCDC_REG_DSIZE_YMUL_MASK) >>
				   DEVLCDC_REG_DSIZE_YMUL_SHIFT) + 1;
			DEBUG("DevLCDC: Size %d x %d (scale %dx%d)\n",
			      display_x, display_y, scale_x, scale_y);
			if (style == STYLE_PDP && (display_x != 640 || display_y != 480)) {
				WARN("DevLCDC: Configured incompatible dimensions %dx%d for PDP\n",
				     display_x, display_y);
			}
			// FIXME, notify change/rebuild window/surface if necessary
			break;
		case DEVLCDC_REG_VSYNC:
			regs[reg] = data & (DEVLCDC_REG_VSYNC_WIDTH_MASK |
					    DEVLCDC_REG_VSYNC_FP_MASK |
					    DEVLCDC_REG_VSYNC_BP_MASK |
					    DEVLCDC_REG_VSYNC_POL_MASK);
			break;
		case DEVLCDC_REG_HSYNC:
			regs[reg] = data & (DEVLCDC_REG_HSYNC_WIDTH_MASK |
					    DEVLCDC_REG_HSYNC_FP_MASK |
					    DEVLCDC_REG_HSYNC_BP_MASK |
					    DEVLCDC_REG_HSYNC_POL_MASK |
					    DEVLCDC_REG_HSYNC_DE_POL_MASK |
					    DEVLCDC_REG_HSYNC_BLK_POL_MASK
				);
			break;
		case DEVLCDC_REG_FB_BASE:
			regs[reg] = data;
			// And notify new change
			break;
		case DEVLCDC_REG_DWPL_BPP:
			regs[reg] = data & (DEVLCDC_REG_DWPL_BPP_DWPL_MASK |
					    DEVLCDC_REG_DWPL_BPP_BPP_MASK);
			bpp = 1 << ((data & DEVLCDC_REG_DWPL_BPP_BPP_MASK) >> DEVLCDC_REG_DWPL_BPP_BPP_SHIFT);
			DEBUG("DevLCDC: %d BPP\n", bpp);
			break;
		case DEVLCDC_REG_PAL_IDX:
			regs[reg] = data & DEVLCDC_REG_PAL_IDX_MASK;
			break;
		case DEVLCDC_REG_PAL_ENTRY:
			// FIXME: If PDP, really it's 4-bit output so palette doesn't
			// store 24-bit values -- clamp it here.
			palette[regs[DEVLCDC_REG_PAL_IDX]] = data & DEVLCDC_REG_PAL_ENTRY_MASK;
			break;

		default:
			FATAL("DevLCDC: Write of 0x%08x to undef reg, addr 0x%08x\n", data, addr);
	}
}

void	DevLCDC::write16(PA addr, u16 data)
{
	FATAL("DevLCDC::write16(" FMT_PA ", %04x)\n", addr, data);
}

void	DevLCDC::write8(PA addr, u8 data)
{
	FATAL("DevLCDC::write8(" FMT_PA ", %04x)\n", addr, data);
}

////////////////////////////////////////////////////////////////////////////////
// SDL stuff

#ifndef NO_SDL
void 	DevLCDC::copyLine32bpp(uint32_t *lineout, unsigned int width, unsigned int pscale, uint32_t **in)
{
	for (unsigned int x = 0; x < width; x++) {
		uint32_t pix = be32toh(**in);
		(*in)++;
		for (unsigned int s = 0; s < pscale; s++) {
			*lineout++ = pix;
		}
	}
}

void 	DevLCDC::copyLine16bpp(uint32_t *lineout, unsigned int width, unsigned int pscale, uint32_t **in)
{
	unsigned int pcount = 0;
	uint32_t opx = 0;
	uint8_t r, g, b;

	for (unsigned int x = 0; x < width; x++) {
		if (!pcount) {
			opx = **in;	// "fetch" a chunk of pixels
			(*in)++;
			pcount = 2;	// 2x 16BPP pixels per word
		}
		// BGR565
		r = (opx & 0x001f) << 3; // FIXME, not proper conversion!
		g = (opx & 0x07e0) >> 4;
		b = (opx & 0xf800) >> 8;
		uint32_t pix = (r << 16) | (g << 8) | b;
		for (unsigned int s = 0; s < pscale; s++) {
			*lineout++ = pix;
		}
		opx >>= 16;
		pcount--;
	}
}

void 	DevLCDC::copyLineIndexed(unsigned int bpp, uint32_t *lineout, unsigned int width,
				 unsigned int pscale, uint32_t **in)
{
	unsigned int pcount = 0;
	uint32_t opx = 0;
	uint8_t r, g, b;
	uint32_t pixmask = (1 << bpp) - 1;

	for (unsigned int x = 0; x < width; x++) {
		if (!pcount) {
			opx = **in;
			(*in)++;
			pcount = 32/bpp;	// N pixels per word
		}
		// Palette is BGR
		r = palette[opx & pixmask] & 0xff;
		g = (palette[opx & pixmask] & 0xff00) >> 8;
		b = (palette[opx & pixmask] & 0xff0000) >> 16;
		uint32_t pix = (r << 16) | (g << 8) | b;
		for (unsigned int s = 0; s < pscale; s++) {
			*lineout++ = pix;
		}
		opx >>= bpp;
		pcount--;
	}
}

void 	DevLCDC::transformFramebuffer(SDL_Surface *dest)
{
	uint32_t *ps = (uint32_t *)getAddrDMA(regs[DEVLCDC_REG_FB_BASE]); // FIXME: wrap
	uint32_t *pd = (uint32_t *)dest->pixels;
	/* pd is a 32BPP array: 		display_x*DISP_SCALE by display_y*DISP_SCALE
	 * ps is the source framebuffer:	display_x/scale_x by display_y/scale_y
	 */

	/* Iterate over the framebuffer size, e.g. 320*240 even if window is 640*480
	 * with scale_x/scale_y=x2.  The doubling is then done inline.
	 * There's then also DISP_SCALE which might double/mul it again!
	 */
	for (int y = 0; y < (dest->h/DISP_SCALE/scale_y); y++) {
		uint32_t *lineout = pd;

		/* Cut down on some boilerplate by switching on BPP per line,
		 * but would be most efficient to do it per frame!
		 */
		if (bpp == 32) {
			copyLine32bpp(pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else if (bpp == 16) {
			copyLine16bpp(pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else if (bpp == 8) {
			copyLineIndexed(8, pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else if (bpp == 4) {
			copyLineIndexed(4, pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else if (bpp == 2) {
			copyLineIndexed(2, pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else if (bpp == 1) {
			copyLineIndexed(1, pd, (dest->w/DISP_SCALE/scale_x), DISP_SCALE*scale_x, &ps);
		} else {
			static bool warned = false;
			if (!warned) {
				WARN("DevLCDC: Weird BPP %d!\n", bpp);
				warned = true;
			}
			for (int x = 0; x < dest->w; x++) {
				*lineout++ = 0x000000ff; // Blue screen
			}
		}
		// Write N lines:
		for (int s = 1; s < DISP_SCALE*scale_y; s++) {
			memcpy(pd + (dest->w*s), pd, dest->w*4);
		}
		pd += dest->w * DISP_SCALE * scale_y;  // Move ptr up N lines
	}
}
#endif

void	DevLCDC::init()
{
#ifndef NO_SDL
	/* Initialise SDL */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		FATAL("DevLCDC: SDL init failed!\n");
	}

	window = SDL_CreateWindow("DevLCDC",
				  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				  DISP_WIDTH, DISP_HEIGHT,
				  SDL_WINDOW_SHOWN
		);
	if (!window) {
		FATAL("DevLCDC: Can't create SDL window!\n");
	}

	screen = SDL_GetWindowSurface(window);
	if (!screen) {
		FATAL("DevLCDC: Can't create SDL window surface!\n");
	}
	// Seems to default to 32BPP with 0RGB order.
#endif
	frame_end_time = 0;

	/* Initialise registers */
	for (int i = 0; i <= DEVLCDC_REG_END; i++) {
		regs[i] = 0;
	}

	init_done = true;
}

/* Periodic callback: Check for events and update the screen.
 */
void	DevLCDC::periodic(u64 ticks)
{
#ifdef NO_SDL
	return;
#else
	SDL_Event event;

	if (!init_done)
		return;

	if (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				done = true;
				break;

				// Channel keypresses into input layer
				// (eventually provide serio)
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					default:
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					default:
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				// event.motion.x event.motion.y
			default:
				break;
		}
	}

	if (done) {
		sim_quit();
	} else if (display_enable) {
		transformFramebuffer(screen);
		SDL_UpdateWindowSurface(window);
	}
#endif
}

void	DevLCDC::setStyle(int s)
{
	switch (s) {
		case STYLE_PDP:
			// TODO: Limit display to 640x480 (not configurable)
			// Support 1,2,4,8,16?,32?BPP (palette somehow mapped to 4BPP out)
			// Apply orange tint :)
		case STYLE_HDMI:
			// Resize window when resolution change occurs
			// Map colours 1,2,4,8,16,32bpp
		default:
			break;
	}
}

void 	*DevLCDC::getAddrDMA(PA addr)
{
	void *host_addr;
	if (!bus) {
		FATAL("DevLCDC::getAddrDMA: Needs bus setup\n");
	} else {
		if (bus->get_direct_map(addr, &host_addr)) {
			return host_addr;
		} else {
			FATAL("DevLCDC::getAddrDMA: Can't direct-map PA %08x\n", addr);
		}
	}
	return 0;
}
