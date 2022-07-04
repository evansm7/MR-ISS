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
#include "types.h"

/* Defines the enum for all counters, plus COUNTER_MAX. */
#include "stats_counter_defs.h"

u64	global_counters[COUNTER_MAX];

void	stats_dump(void)
{
#if ENABLE_COUNTERS > 0
	printf("Global counters:\n");
	for (int i = COUNTER_START+1; i < COUNTER_MAX; i++) {
		printf("\t%40s:\t%lld\n",
		       global_counter_names[i-1],
		       global_counters[i]);
	}
#endif
}

void	stats_reset(void)
{
#if ENABLE_COUNTERS > 0
	for (int i = COUNTER_START+1; i < COUNTER_MAX; i++) {
		global_counters[i] = 0;
	}
#endif
}
