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

#ifndef SIM_STATE_H
#define SIM_STATE_H

#include "types.h"
#include "PPCCPUState.h"
#include "log.h"
#include "platform.h"

int	state_save_open(PPCCPUState *pcs, meminfo_t meminfo[], unsigned int membanks);
void	state_save_finalise(int handle);

// State save chunk header:

typedef struct {
	u64 	name;
	u64 	len; // Excluding header, in bytes (rounded to 8)
	u64 	data;
} ss_chunk_t;


// Chunk types

#define STS_GPR_FMT	"GPR%02d"
#define STS_SR_FMT	"SR%02d"
#define STS_MEM		"MEMBLK"

#define STS_MEM_CHUNK_SZ	0x200000


#define	SAVE_REG_CHUNK(f, val, cname)	do {                            \
                ss_chunk_t _sst;                                        \
		strncpy((char *)&(_sst.name), (cname), 8);		\
		_sst.len = 8;						\
		_sst.data = (val);					\
		int r = write((f), &_sst, 24);				\
		if (r != 24) {						\
			WARN("save_state: write failed (%d)\n", r);	\
			goto err;                                       \
                }                                                       \
	} while (0)


#endif
