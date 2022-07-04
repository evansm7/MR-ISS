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

#ifndef DEVRAM_H
#define DEVRAM_H

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __DARWIN__
// FIXME, OS X specific:
#include <mach/vm_param.h>
#endif

#include "log.h"

#define ALIGNMENT_HAX 1

class DevRAM : public Device {
public:
	DevRAM(bool actuallyROM = false) : isROM(actuallyROM), mmap_base(0), fnam(0) {}

	u32	read32(PA addr)
	{
#ifdef ALIGNMENT_HAX
		u32 d = *((u32 *)(mmap_base + offset_from_PA(addr)));
#else
		u32 d = ((u32 *)mmap_base)[offset_from_PA(addr) >> 2];
#endif
		DEBUG("- R%cM RD[" FMT_PA "] = %08x\n", isROM ? 'O' : 'A', addr, d);
		return d;
	}

	u16	read16(PA addr)
	{
#ifdef ALIGNMENT_HAX
		u16 d = *((u16 *)(mmap_base + offset_from_PA(addr)));
#else
		u16 d = ((u16 *)mmap_base)[offset_from_PA(addr) >> 1];
#endif
		DEBUG("- R%cM RD[" FMT_PA "] = %04x\n", isROM ? 'O' : 'A', addr, d);
		return d;
	}

	u8	read8(PA addr)
	{
		u8 d = ((u8 *)mmap_base)[offset_from_PA(addr)];
		DEBUG("- R%cM RD[" FMT_PA "] = %02x\n", isROM ? 'O' : 'A', addr, d);
		return d;
	}

	void	write32(PA addr, u32 data)
	{
		DEBUG("- R%cM WR[" FMT_PA "] <= %08x\n", isROM ? 'O' : 'A', addr, data);
		if (!isROM)
#ifdef ALIGNMENT_HAX
			*((u32 *)(mmap_base + offset_from_PA(addr))) = data;
#else
			((u32 *)mmap_base)[offset_from_PA(addr) >> 2] = data;
#endif
	}

	void	write16(PA addr, u16 data)
	{
		DEBUG("- R%cM WR[" FMT_PA "] <= %04x\n", isROM ? 'O' : 'A', addr, data);
		if (!isROM)
#ifdef ALIGNMENT_HAX
			*((u16 *)(mmap_base + offset_from_PA(addr))) = data;
#else
			((u16 *)mmap_base)[offset_from_PA(addr) >> 1] = data;
#endif
	}

	void	write8(PA addr, u8 data)
	{
		DEBUG("- R%cM WR[" FMT_PA "] <= %02x\n", isROM ? 'O' : 'A', addr, data);
		if (!isROM)
			((u8 *)mmap_base)[offset_from_PA(addr)] = data;
	}

	void	setProps(PA addr, PA size)
	{
		base_address = addr;
		// Fugly.  Shouldn't try to set it twice.
//		address_span = size;
	}

	void	dump()
	{
		if (isROM)
			DEBUG("ROM ");
		else
			DEBUG("RAM ");

		if (fnam) {
			DEBUG("from file '%s'", fnam);
		}
		DEBUG("\n");
	}

	bool	direct_map(PA addr, void **out_addr)
	{
		*out_addr = (void *)(mmap_base + offset_from_PA(addr));
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Local functions:
	// save_to_file.

	// Map the memory as a plain ANON chunk.
	void 	init(unsigned int size)
	{
		base_address = 0;
		address_span = size;

		// Plain old anon mmap for entire range.
		mmap_base = (u8 *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if (mmap_base == MAP_FAILED) {
			FATAL("Cannot anon-mmap() %d bytes of memory\n", size);
		}
	}

	void	loadAtOffset(char *from_filename, PA offset)
	{
		/* Simply read() the file into the existing mmap */
		if (!mmap_base) {
			FATAL("RAM: Not initialised before loadAtOffset\n");
		}

		int fd = open(from_filename, O_RDONLY);
		if (fd < 0) {
			FATAL("RAM: Can't open image '%s'!\n", from_filename);
		}
		// Stat to get size:
		struct stat sb;
		if (fstat(fd, &sb) < 0) {
			perror("Can't stat fd");
		}

		if (sb.st_size + offset > address_span) {
			FATAL("RAM: File goes off end of RAM (RAM size 0x%x, file size 0x%lx, offset 0x%x)\n",
			      address_span, sb.st_size, offset);
		}

		int r = read(fd, mmap_base + offset, sb.st_size);
		if (r < 0) {
			perror("RAM read:");
			FATAL("Failed to load RAM contents\n");
		}
		if (r < sb.st_size) {
			FATAL("RAM: Only loaded %d of %d\n", r, sb.st_size);
		}
		close(fd);

		if (fnam != 0)
			free(fnam);
		fnam = strdup(from_filename);
	}

	u8	*getMapBase()
	{
		return mmap_base;
	}

private:
	PA	offset_from_PA(PA addr)
	{
		return addr - base_address;
	}

private:
	bool	isROM;
	u8	*mmap_base;
	char 	*fnam;			// For debug
	// Types:
	PA	base_address;
	PA	address_span;
};

#endif
