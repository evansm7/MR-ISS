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

#ifndef DEVLCDC_H
#define DEVLCDC_H

#ifndef NO_SDL
#include <SDL.h>
#endif
#include "Device.h"
#include "log.h"
#include "AbstractIntc.h"
#include "Bus.h"


#define DEVLCDC_REG_STAT_CTRL		0	// @0x00
#define DEVLCDC_REG_STAT_CTRL_BEN	0x40000000
#define DEVLCDC_REG_STAT_CTRL_EN	0x80000000
/* Display position not supported */
#define DEVLCDC_REG_DSIZE		1	// @0x04
#define DEVLCDC_REG_DSIZE_WIDTH_SHIFT	0
#define DEVLCDC_REG_DSIZE_WIDTH_MASK	0x7ff
#define DEVLCDC_REG_DSIZE_XMUL_SHIFT	11
#define DEVLCDC_REG_DSIZE_XMUL_MASK	0xf800
#define DEVLCDC_REG_DSIZE_HEIGHT_SHIFT	16
#define DEVLCDC_REG_DSIZE_HEIGHT_MASK	0x7ff0000
#define DEVLCDC_REG_DSIZE_YMUL_SHIFT	27
#define DEVLCDC_REG_DSIZE_YMUL_MASK	0xf8000000

#define DEVLCDC_REG_VSYNC		2	// @0x08
#define DEVLCDC_REG_VSYNC_WIDTH_SHIFT	0
#define DEVLCDC_REG_VSYNC_WIDTH_MASK	0xff
#define DEVLCDC_REG_VSYNC_FP_SHIFT	8
#define DEVLCDC_REG_VSYNC_FP_MASK	0xff00
#define DEVLCDC_REG_VSYNC_BP_SHIFT	16
#define DEVLCDC_REG_VSYNC_BP_MASK	0xff0000
#define DEVLCDC_REG_VSYNC_POL_MASK	0x1000000

#define DEVLCDC_REG_HSYNC		3	// @0x0c
#define DEVLCDC_REG_HSYNC_WIDTH_SHIFT	0
#define DEVLCDC_REG_HSYNC_WIDTH_MASK	0xff
#define DEVLCDC_REG_HSYNC_FP_SHIFT	8
#define DEVLCDC_REG_HSYNC_FP_MASK	0xff00
#define DEVLCDC_REG_HSYNC_BP_SHIFT	16
#define DEVLCDC_REG_HSYNC_BP_MASK	0xff0000
#define DEVLCDC_REG_HSYNC_POL_MASK	0x1000000
#define DEVLCDC_REG_HSYNC_DE_POL_MASK	0x2000000
#define DEVLCDC_REG_HSYNC_BLK_POL_MASK	0x4000000

#define DEVLCDC_REG_FB_BASE		4	// @0x10

#define DEVLCDC_REG_ID			5	// @0x14
#define DEVLCDC_REG_ID_VAL		0x44430001

#define DEVLCDC_REG_DWPL_BPP		6	// @0x18
#define DEVLCDC_REG_DWPL_BPP_DWPL_SHIFT 0
#define DEVLCDC_REG_DWPL_BPP_DWPL_MASK	0x7ff
#define DEVLCDC_REG_DWPL_BPP_BPP_SHIFT 	11
#define DEVLCDC_REG_DWPL_BPP_BPP_MASK 	0x3800

#define DEVLCDC_REG_PAL_IDX		7	// @0x1c
#define DEVLCDC_REG_PAL_IDX_MASK	0xff

#define DEVLCDC_REG_PAL_ENTRY		8	// @0x20
#define DEVLCDC_REG_PAL_ENTRY_MASK	0xffffff

#define DEVLCDC_REG_END			8


class DevLCDC : public Device {
public:
        DevLCDC() : base_address(0), address_span(0), bus(0), intc(0), init_done(false), done(false),
		    style(STYLE_HDMI), display_enable(false), border_enable(false),
		    display_x(640), display_y(480),
		    scale_x(1), scale_y(1), bpp(0)
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
		DEBUG("DevLCDC\n");
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

	// Quick hack:  PDP is orange and fixed size; HDMI is colour & variable
	void	setStyle(int s);
	static const int STYLE_PDP = 0;
	static const int STYLE_HDMI = 1;

	// Platform provides periodic callbacks:
	void	periodic(u64 ticks);

private:
	void	init();
	void 	*getAddrDMA(PA addr);
#ifndef NO_SDL
	void	transformFramebuffer(SDL_Surface *dest);
	void 	copyLine32bpp(uint32_t *lineout, unsigned int width, unsigned int pscale, uint32_t **in);
	void 	copyLine16bpp(uint32_t *lineout, unsigned int width, unsigned int pscale, uint32_t **in);
	void 	copyLineIndexed(unsigned int bpp, uint32_t *lineout, unsigned int width,
				unsigned int pscale, uint32_t **in);
#endif

	// Types:
	PA	base_address;
	PA	address_span;

	Bus		*bus;
	AbstractIntc 	*intc;
	unsigned int	irq_number;
	bool		init_done;
	bool		done;
	uint32_t	frame_end_time;

#ifndef NO_SDL
	SDL_Window	*window;
	SDL_Surface 	*screen;
#endif

	//
	u32		regs[8];
	u32		palette[256];
	int		style;
	bool		display_enable;
	bool		border_enable;
	unsigned int	display_x;
	unsigned int	display_y;
	unsigned int	scale_x;
	unsigned int	scale_y;
	u8		bpp;
};

#endif
