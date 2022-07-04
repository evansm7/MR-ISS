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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"
#include "sim_state.h"
#include "PPCMMU.h"
#include "PPCCPUState.h"
#include "log.h"
#include "Config.h"


int state_save_open(PPCCPUState *pcs, meminfo_t meminfo[], unsigned int membanks)
{
	int 		fd;
	char 	       *filename = CFG(save_state_path);

	if (!filename)
		return -1;	// Nothing saved

	fd = open(filename, O_RDWR | O_CREAT, 0644);

	if (fd < 0) {
		WARN("state_save: Can't open %s\n", filename);
		return -1;
	}

	for (int i = 0; i < 32; i++) {
		char name[8];
		snprintf(name, 8, STS_GPR_FMT, i);
		SAVE_REG_CHUNK(fd, pcs->getGPR(i), name);
	}

	// Core regs
	SAVE_REG_CHUNK(fd, pcs->getPC(), "PC");
	SAVE_REG_CHUNK(fd, pcs->getCTR(), "CTR");
	SAVE_REG_CHUNK(fd, pcs->getLR(), "LR");
	SAVE_REG_CHUNK(fd, pcs->getXER(), "XER");
	SAVE_REG_CHUNK(fd, pcs->getCR(), "CR");
	SAVE_REG_CHUNK(fd, pcs->getMSR(), "MSR");
	SAVE_REG_CHUNK(fd, pcs->getHID0(), "HID0"); // Aren't these RES0?
	SAVE_REG_CHUNK(fd, pcs->getHID1(), "HID1");
	SAVE_REG_CHUNK(fd, pcs->getSPRG0(), "SPRG0");
	SAVE_REG_CHUNK(fd, pcs->getSPRG1(), "SPRG1");
	SAVE_REG_CHUNK(fd, pcs->getSPRG2(), "SPRG2");
	SAVE_REG_CHUNK(fd, pcs->getSPRG3(), "SPRG3");
	SAVE_REG_CHUNK(fd, pcs->getSRR0(), "SRR0");
	SAVE_REG_CHUNK(fd, pcs->getSRR1(), "SRR1");
	SAVE_REG_CHUNK(fd, pcs->getDAR(), "DAR");
	SAVE_REG_CHUNK(fd, pcs->getDSISR(), "DSISR");
	SAVE_REG_CHUNK(fd, pcs->getDEC(), "DEC");
	SAVE_REG_CHUNK(fd, pcs->getTB(), "TB");

	// MMU
	SAVE_REG_CHUNK(fd, pcs->getSDR1(), "SDR1");

	for (int i = 0; i < 16; i++) {
		char name[8];
		snprintf(name, 8, STS_SR_FMT, i);
		SAVE_REG_CHUNK(fd, pcs->getMMU()->getSegmentReg(i), name);
	}

	for (int i = 0; i < 4; i++) {
		char name[8];

		snprintf(name, 8, "IBAT%dU", i);
		SAVE_REG_CHUNK(fd, pcs->getMMU()->getIBATupper(i), name);
		snprintf(name, 8, "IBAT%dL", i);
		SAVE_REG_CHUNK(fd, pcs->getMMU()->getIBATlower(i), name);
		snprintf(name, 8, "DBAT%dU", i);
		SAVE_REG_CHUNK(fd, pcs->getMMU()->getDBATupper(i), name);
		snprintf(name, 8, "DBAT%dL", i);
		SAVE_REG_CHUNK(fd, pcs->getMMU()->getDBATlower(i), name);
	}

	// External IRQ input
	SAVE_REG_CHUNK(fd, pcs->getRawIRQPending(), "IRQ");

	// Memory in 2M chunks
        for (unsigned int b = 0; b < membanks; b++) {
                for (unsigned int i = 0; i < meminfo[b].ram_size;
                     i += STS_MEM_CHUNK_SZ) {
                        u8 *chunk_base = meminfo[b].host_map_base + i;

                        // Is there anything non-zero?
                        u64 v = 0;
                        for (unsigned int j = 0; j < STS_MEM_CHUNK_SZ; j += 8)
                                v |= ((u64 *)chunk_base)[j/8];

                        if (v) {
                                ss_chunk_t	smh;
                                int r;

                                strncpy((char *)&smh.name, STS_MEM, 8);
                                smh.len = 8 + STS_MEM_CHUNK_SZ; // 2M plus 1 dword for base
                                smh.data = meminfo[b].ram_pa_base + i; // Data is base addr, mem follows

                                r = write(fd, &smh, sizeof(smh));
                                if (r != sizeof(smh)) {
                                        WARN("save_state: write failed (%d)\n", r);
                                        goto err;
                                }
                                r = write(fd, chunk_base, STS_MEM_CHUNK_SZ);
                                if (r != STS_MEM_CHUNK_SZ) {
                                        WARN("save_state: write failed (%d)\n", r);
                                        goto err;
                                }
                        }
                }
        }

	return fd;

err:
        close(fd);
        return -1;
}

void state_save_finalise(int handle)
{
	close(handle);
}


