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

#include "DevDebug.h"

u32	DevDebug::read32(PA addr)
{
	FATAL("DevDebug::read32(%x)\n", addr);
	return 0;
}

u16	DevDebug::read16(PA addr)
{
	FATAL("DevDebug::read16(%x)\n", addr);
	return 0;
}

u8	DevDebug::read8(PA addr)
{
	FATAL("DevDebug::read8(%x)\n", addr);
	return 0;
}

void	DevDebug::write32(PA addr, u32 data)
{
	FATAL("DevDebug::write32(%x, %x)\n", addr, data);
}

void	DevDebug::write16(PA addr, u16 data)
{
	FATAL("DevDebug::write16(%x, %x)\n", addr, data);
}

void	DevDebug::write8(PA addr, u8 data)
{
	FATAL("DevDebug::write8(%x, %x)\n", addr, data);
}

