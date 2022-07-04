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

#include "DevDummy.h"

u32	DevDummy::read32(PA addr)
{
	IOTRACE("DevDummy::read32(%x)\n", addr);
	return 0;
}

u16	DevDummy::read16(PA addr)
{
	IOTRACE("DevDummy::read16(%x)\n", addr);
	return 0;
}

u8	DevDummy::read8(PA addr)
{
	IOTRACE("DevDummy::read8(%x)\n", addr);
	return 0;
}

void	DevDummy::write32(PA addr, u32 data)
{
	IOTRACE("DevDummy::write32(%x, %x)\n", addr, data);
}

void	DevDummy::write16(PA addr, u16 data)
{
	IOTRACE("DevDummy::write16(%x, %x)\n", addr, data);
}

void	DevDummy::write8(PA addr, u8 data)
{
	IOTRACE("DevDummy::write8(%x, %x)\n", addr, data);
}

