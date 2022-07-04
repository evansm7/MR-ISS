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

#ifndef DEVSIMPLEPIPE_H
#define DEVSIMPLEPIPE_H

#include "log.h"

class DevSimplePipe : public Device {
public:
        DevSimplePipe() : base_address(0), address_span(0)
	{
	}

	u32	read32(PA addr)
	{
		return 0;
	}

	u16	read16(PA addr)
	{
		return 0;
	}

	u8	read8(PA addr)
	{
		return 0;
	}

	void	write32(PA addr, u32 data)
	{
		DEBUG(": devsp write32(" FMT_PA ", %08x)\n", addr, data);
		printf("%c", data);
	}

	void	write16(PA addr, u16 data)
	{
		DEBUG(": devsp write16(" FMT_PA ", %04x)\n", addr, data);
		printf("%c", data);
	}

	void	write8(PA addr, u8 data)
	{
		DEBUG(": devsp write8(" FMT_PA ", %02x)\n", addr, data);
		printf("%c", data);
	}

	void	setProps(PA addr, PA size)
	{
		base_address = addr;
		address_span = size;
	}

	void	dump()
	{
		DEBUG("DevSimplePipe\n");
	}
	
private:
	// Types:
	PA	base_address;
	PA	address_span;
};

#endif
