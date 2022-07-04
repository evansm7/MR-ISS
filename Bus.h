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

#ifndef BUS_H
#define BUS_H

#include "types.h"
#include "log.h"
#include "Device.h"

class Bus
{
public:
        Bus() : last_dev(0) {};
private:
	static const int bus_entries = 32;

	class BusDev
	{
	public:
	        BusDev() : addr(0), size(0), dev(0) {};

		void	setBase(PA addr);
		void	setSize(PA size);

		PA	addr;
		PA	size;

		Device	*dev;
	};

	BusDev	devs[bus_entries];
	int	last_dev;

	bool	dev_matches_addr(int dev, PA addr)
	{
		return (addr >= devs[dev].addr && addr < (devs[dev].addr + devs[dev].size));
	}

	int	find_dev_for_addr(PA addr)
	{
		for (int i = 0; i < bus_entries; i++) {
			if (dev_matches_addr(i, addr))
				return i;
		}
		return -1;
	}

	int	get_dev(PA addr)
	{
		int i = 0;
		if (dev_matches_addr(last_dev, addr)) {
			i = last_dev;
		} else {
			i = last_dev = find_dev_for_addr(addr);
			if (i < 0) {
				FATAL("Access to addr %08x missed all devices\n", addr);
			}
		}
		return i;
	}

	////////////////////////////////////////////////////////////////////////////////
public:
	void	attach(Device *dev, PA base, PA size)
	{
		for (int i = 0; i < bus_entries; i++) {
			if (devs[i].dev == 0) {
				devs[i].dev = dev;
				devs[i].addr = base;
				devs[i].size = size;
				devs[i].dev->setProps(base, size);
				return;
			}
		}
		FATAL("Can't find free dev to attach.\n");
	}

	// Find device, then call read/write on it.
	u32	read32(PA addr)
	{
		int i = get_dev(addr);
		return devs[i].dev->read32(addr);
	}

	u16	read16(PA addr)
	{
		int i = get_dev(addr);
		return devs[i].dev->read16(addr);
	}

	u8	read8(PA addr)
	{
		int i = get_dev(addr);
		return devs[i].dev->read8(addr);
	}

        void	write32(PA addr, u32 data)
	{
		int i = get_dev(addr);
		devs[i].dev->write32(addr, data);
	}

        void	write16(PA addr, u16 data)
	{
		int i = get_dev(addr);
		devs[i].dev->write16(addr, data);
	}

	void	write8(PA addr, u8 data)
	{
		int i = get_dev(addr);
		devs[i].dev->write8(addr, data);
	}

	bool	get_direct_map(PA addr, void **map_at)
	{
		int i = get_dev(addr);
		return devs[i].dev->direct_map(addr, map_at);
	}

	void	dump()
	{
		printf("Bus devices:\n");
		for (int i = 0; i < bus_entries; i++) {
			if (devs[i].size != 0) {
				printf("	%02d:	%08x - %08x	%p\n",
				       i, devs[i].addr, devs[i].addr + devs[i].size, devs[i].dev);
			}
		}
	}
};

#endif
