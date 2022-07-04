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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include "types.h"

#define LOG_FLAG_DBG	1
#define LOG_FLAG_DISASS	2
#define LOG_FLAG_STRACE	4
#define LOG_FLAG_IO_TRACE	8
#define LOG_FLAG_BRANCH_TRACE	16
#define LOG_FLAG_MMU_TRACE	32
#define LOG_FLAG_EXC_TRACE	64
#define LOG_FLAG_JIT_TRACE	128

#define LOG_MSG 	""
#define LOG_WARN	"WARN: "
#define LOG_FATAL	"ERROR: "

#if ENABLE_TRACING > 0
#define DEBUG(x...)	do { if (log_enables_mask & LOG_FLAG_DBG) { lprintf( x ); } } while (0)
#define DISASS(x...)	do { if (log_enables_mask & LOG_FLAG_DISASS) { lprintf( x ); } } while (0)
#define STRACE(x...)	do { if (log_enables_mask & LOG_FLAG_STRACE) { lprintf( "+++ STRACE " x ); } } while (0)
#define IOTRACE(x...)	do { if (log_enables_mask & LOG_FLAG_IO_TRACE) { lprintf( "+++ IO " x ); } } while (0)
#define MMUTRACE(x...)	do { if (log_enables_mask & LOG_FLAG_MMU_TRACE) { lprintf( "+++ MMU " x ); } } while (0)
#define EXCTRACE(x...)	do { if (log_enables_mask & LOG_FLAG_EXC_TRACE) { lprintf( "+++ EXC " x ); } } while (0)
#define JITTRACE(x...)	do { if (log_enables_mask & LOG_FLAG_JIT_TRACE) { lprintf( "+++ JIT " x ); } } while (0)
#define JITTRACING	(!!(log_enables_mask & LOG_FLAG_JIT_TRACE))
#else
#define DEBUG(x...)	do { } while (0)
#define DISASS(x...)	do { } while (0)
#define STRACE(x...)	do { } while (0)
#define IOTRACE(x...)	do { } while (0)
#define MMUTRACE(x...)	do { } while (0)
#define EXCTRACE(x...)	do { } while (0)
#define JITTRACE(x...)	do { } while (0)
#define JITTRACING	(0)
#endif
#define LOG(x...)	do { lprintf( LOG_MSG x ); } while (0)
#define WARN(x...)	do { lprintf( LOG_WARN x); } while (0)
#define FATAL(x...)	do { lprintf( LOG_FATAL x); sim_quit(); } while (0)
#define BRTRACE(x...)	do { if (log_enables_mask & LOG_FLAG_BRANCH_TRACE) { lprintf( "+++ BR " x ); } } while (0)

void 	log_init(void);

extern u32	log_enables_mask;

int	lprintf(const char *format, ...);

extern void sim_quit(void);

#endif
