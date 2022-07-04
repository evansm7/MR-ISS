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

#ifndef DEVICE_H
#define DEVICE_H

#include "types.h"

// Abstract device class with pure virtual interface for devices:
class Device
{
	////////////////////////////////////////////////////////////////////////////////
protected:
	// Cannot be constructed, except for by subclasses
	Device() {};

	////////////////////////////////////////////////////////////////////////////////
public:
	// Any point in supporting multiple types, e.g. smaller accesses?
	virtual	u32	read32(PA addr) = 0;
	virtual	u16	read16(PA addr) = 0;
	virtual	u8	read8(PA addr) = 0;
	virtual	void	write32(PA addr, u32 data) = 0;
	virtual	void	write16(PA addr, u16 data) = 0;
	virtual	void	write8(PA addr, u8 data) = 0;
	virtual void	setProps(PA addr, PA size) = 0;
	virtual void	dump() = 0;

	/* This might get overridden by a subclass */
	virtual bool	direct_map(PA addr, void **out_addr)
	{
		return false;
	}
};

#endif
