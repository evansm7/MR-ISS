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

/* A very simple file-backed block device.
 * This supports only raw image files, one transfer at a time, and currently fully-synchronous operation.
 *
 * Todo:
 * Async completion, IRQ support, descriptor/queue for multiple blocks.
 *
 * 29/08/18 me
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <limits.h>

#include "DevSBD.h"


void	DevSBD::init()
{
	regs[DEVSBD_REG_IDR] = DEVSBD_REG_IDR_ID;
	regs[DEVSBD_REG_BLKS] = 0;	// Updated by openImage()
	regs[DEVSBD_REG_IRQ] = 0;
	regs[DEVSBD_REG_CMD] = 0;
	regs[DEVSBD_REG_PA] = 0;
	regs[DEVSBD_REG_BLK_START] = 0;
	regs[DEVSBD_REG_LEN] = 0;
	regs[DEVSBD_REG_INFO] = (DEVSBD_REG_INFO_BLKSZ_MASK & (DEVSBD_BLK_SIZE/512));
}

u32	DevSBD::read32(PA addr)
{
	u32 data = 0;
	int reg = (addr & 0x1f) >> 2;

	if (reg <= DEVSBD_REG_END) {
		data = regs[reg];
	} else {
		FATAL("DevSBD: Access to undef reg, addr 0x%08x\n", addr);
	}

	IOTRACE("DevSBD: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

u16	DevSBD::read16(PA addr)
{
	FATAL("DevSBD::read16(%x)\n", addr);
	return 0;
}

u8	DevSBD::read8(PA addr)
{
	FATAL("DevSBD::read8(%x)\n", addr);
	return 0;
}

void	DevSBD::write32(PA addr, u32 data)
{
	int reg = (addr & 0x1f) >> 2;

	IOTRACE("DevSBD: WR[%x] => 0x%08x\n", addr, data);

	switch (reg) {
		case DEVSBD_REG_IRQ:
		case DEVSBD_REG_PA:
		case DEVSBD_REG_LEN:
		case DEVSBD_REG_BLK_START:
			regs[reg] = data;
			break;

		case DEVSBD_REG_CMD:
			/* Do something. */
			if (regs[DEVSBD_REG_CMD] != DEVSBD_REG_CMD_IDLE) {
				WARN("DevSBD: Command 0x%x issued, but CMD reg 0x%x -- ignoring write.\n",
				     data, regs[DEVSBD_REG_CMD]);
			} else {
				regs[DEVSBD_REG_CMD] = data;
				// Just do sync shiz for now:
				if (data == DEVSBD_REG_CMD_RD) {
					doRead(regs[DEVSBD_REG_BLK_START],
					       regs[DEVSBD_REG_LEN],
					       regs[DEVSBD_REG_PA]);
					// FIXME: synchronous completion
					regs[DEVSBD_REG_CMD] = DEVSBD_REG_CMD_IDLE;
				} else if (data == DEVSBD_REG_CMD_WR) {
					doWrite(regs[DEVSBD_REG_BLK_START],
						regs[DEVSBD_REG_LEN],
						regs[DEVSBD_REG_PA]);
					// FIXME: synchronous completion
					regs[DEVSBD_REG_CMD] = DEVSBD_REG_CMD_IDLE;
				} else {
					WARN("DevSBD: Command 0x%x issued, unrecognised.  Ignored.\n", data);
					regs[DEVSBD_REG_CMD] = DEVSBD_REG_CMD_IDLE;
				}
			}
			break;

		default:
			do {} while(0); /* Other regs WI */
	}

}

void	DevSBD::write16(PA addr, u16 data)
{
	FATAL("DevSBD::write16(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSBD::write8(PA addr, u8 data)
{
	FATAL("DevSBD::write8(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSBD::openImage(char *filename)
{
	fd = open(filename, O_RDWR);

	if (fd < 0) {
		perror("DevSBD::openImage open");
		return;
	}

	struct stat sb;

	if (fstat(fd, &sb) < 0) {
		perror("DevSBD::openImage fstat");
		close(fd);
		return;
	}

	u64	blks = sb.st_size / DEVSBD_BLK_SIZE;

	if (blks > UINT_MAX) {
		WARN("DevSBD::openImage: Image %s is too big (%lld blocks), disabling.\n", blks);
		close(fd);
		return;
	} else {
		nr_blocks = (u32)blks;
		regs[DEVSBD_REG_BLKS] = nr_blocks;
	}

	LOG("DevSBD:  Opened image '%s', %d blocks (%lld bytes)\n", filename, blks, sb.st_size);
}

void	DevSBD::doRead(u32 block_start, u32 num, PA to)
{
	if (num == 0) {
		WARN("DevSbd::doRead:  Zero-length read requested, block %d\n", block_start);
		return;
	}

	if (fd >= 0) {
		void *ha = getAddrDMA(to);

		IOTRACE("DevSBD: Read block %d, num %d, to PA %08x (host %p)\n",
			block_start, num, to, ha);

		if ((block_start >= nr_blocks) ||
		    ((block_start + num) > nr_blocks)) {
			WARN("DevSBD::doRead: block read at %d+%d, off end of device (%d blocks)\n",
			     block_start, num, nr_blocks);
			return;
		}

		// Do read:
		if (pread(fd, ha, num*DEVSBD_BLK_SIZE, block_start*DEVSBD_BLK_SIZE) < 0) {
			perror("DevSBD::doRead pread");
			// Then what?  Do nothing?
		}
	} else {
		WARN("DevSbd::doRead: Read block %d, but no image open.\n", block_start);
		return;
	}
}

void	DevSBD::doWrite(u32 block_start, u32 num, PA from)
{
	if (num == 0) {
		WARN("DevSbd::doWrite: Zero-length write requested, block %d\n", block_start);
		return;
	}

	if (fd >= 0) {
		void *ha = getAddrDMA(from);

		IOTRACE("DevSBD: Write block %d, num %d, from PA %08x (host %p)\n",
			block_start, num, from, ha);

		if ((block_start >= nr_blocks) ||
		    ((block_start + num) > nr_blocks)) {
			WARN("DevSBD::doWrite: block write at %d+%d, off end of device (%d blocks)\n",
			     block_start, num, nr_blocks);
			return;
		}

		// Do write:
		if (pwrite(fd, ha, num*DEVSBD_BLK_SIZE, block_start*DEVSBD_BLK_SIZE) < 0) {
			perror("DevSBD::doWrite pwrite");
			// Then what?  Do nothing?
		}
	} else {
		WARN("DevSbd::doWrite: Write block %d, but no image open.\n", block_start);
		return;
	}
}

void 	*DevSBD::getAddrDMA(PA addr)
{
	void *host_addr;
	if (!bus) {
		FATAL("DevSBD::getAddrDMA: Needs bus setup\n");
	} else {
		if (bus->get_direct_map(addr, &host_addr)) {
			return host_addr;
		} else {
			FATAL("DevSBD::getAddrDMA: Can't direct-map PA %08x\n", addr);
		}
	}
	return 0;
}
