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

#include "log.h"
#include "Config.h"

#include <stdarg.h>

FILE 	*log_str = stderr;
u32	log_enables_mask = 0;

void 	log_init(void)
{
	if (CFG(verbose)) {
		log_enables_mask |= LOG_FLAG_DBG;
	}
	if (CFG(disass)) {
		log_enables_mask |= LOG_FLAG_DISASS;
	}
	if (CFG(strace)) {
		log_enables_mask |= LOG_FLAG_STRACE;
	}
	if (CFG(io_trace)) {
		log_enables_mask |= LOG_FLAG_IO_TRACE;
	}
	if (CFG(branch_trace)) {
		log_enables_mask |= LOG_FLAG_BRANCH_TRACE;
	}
	if (CFG(mmu_trace)) {
		log_enables_mask |= LOG_FLAG_MMU_TRACE;
	}
	if (CFG(exception_trace)) {
		log_enables_mask |= LOG_FLAG_EXC_TRACE;
	}
	if (CFG(jit_trace)) {
		log_enables_mask |= LOG_FLAG_JIT_TRACE;
	}

	/* For now just log to stderr */

	log_str = fdopen(2, "w");
	if (!log_str)
		fprintf(stderr, "Can't init log stream, wtf?\n");
}

int 	lprintf(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	vfprintf(log_str, format, va);
	va_end(va);
	return 0;
}
