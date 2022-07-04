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

#include "Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Config
static Config		config;

void 	Config::usage(char *prog)
{
	fprintf(stderr, "Usage:  %s <opts>\n", prog);
	fprintf(stderr, "\t-h \t\t This help\n"
		"\t-r <path> \t Set RAM image path (default %s)\n"
		"\t-L <addr> \t Set RAM image load address\n"
		"\t-b <path> \t Set blockdev image path (default off)\n"
		"\t\t\t Provide up to %d times for additional discs\n"
		"\t-v \t\t Set verbose\n"
		"\t-d \t\t Set disassembly on\n"
		"\t-l <limit> \t Set instruction count limit\n"
		"\t-p <num> \t Set periodic state dump\n"
		"\t-s <addr> \t Set start address\n"
		"\t-m <MSR> \t Set start MSR value (e.g. 0x40 for high vecs)\n"
                "\t-G <num> \t Set GPIO inputs (default 0x80000000 for sim)\n"
		"\t-x <path> \t Save state at exit to file\n"
		"\t-t <trace type> \t Enable trace:\n"
		"\t\t\tsyscall \t Syscall trace\n"
		"\t\t\tio \t\t IO trace\n"
		"\t\t\tbranch \t\t Branch trace\n"
		"\t\t\tmmu \t\t MMU trace\n"
		"\t\t\texception \t Exception trace\n"
		"\t\t\tjit \t\t JIT trace\n"
/* Future:
 *
 * -B <address>,<action>	Set breakpoint/action: start disasm, stop disasm, stop sim, dump state
 */
		, rom_path, SBD_NUM);
}

void	Config::setup(int argc, char *argv[])
{
	int ch;
	while ((ch = getopt(argc, argv, "hr:vdl:p:s:L:t:b:m:x:G:")) != -1) {
		switch (ch) {
			case 'r':
				rom_path = strdup(optarg);	/* Memory leak */
				break;
			case 'v':
				verbose = true;
				break;
			case 'd':
				disass = true;
				break;
			case 'l':
				instr_limit = strtoull(optarg, NULL, 0);
				break;
			case 'p':
				dump_state_period = strtoul(optarg, NULL, 0);
				break;
			case 's':
				start_addr = strtoull(optarg, NULL, 0);
				break;
			case 'm':
				start_msr = strtoull(optarg, NULL, 0);
				break;
			case 'L':
				load_addr = strtoull(optarg, NULL, 0);
				break;
			case 'G':
				gpio_inputs = strtoull(optarg, NULL, 0);
				break;
			case 'b': {
				int assigned = 0;
				for (int i = 0; i < SBD_NUM; i++) {
					if (block_path[i] == 0) {
						assigned = 1;
						block_path[i] = strdup(optarg);	/* Memory leak */
						break;
					}
				}
				if (!assigned)
					fprintf(stderr, "Too many SBDs (max %d), cannot attach '%s'\n",
						SBD_NUM, optarg);
			} break;

			case 't': {
				/* Parse tracing request */
				if (!strcasecmp(optarg, "syscall")) {
					strace = true;
				} else if (!strcasecmp(optarg, "io")) {
					io_trace = true;
				} else if (!strcasecmp(optarg, "branch")) {
					branch_trace = true;
				} else if (!strcasecmp(optarg, "mmu")) {
					mmu_trace = true;
				} else if (!strcasecmp(optarg, "exception")) {
					exception_trace = true;
				} else if (!strcasecmp(optarg, "jit")) {
					jit_trace = true;
				} else {
					fprintf(stderr, "Unknown trace type '%s'\n", optarg);
					usage(argv[0]);
					exit(1);
				}
			} break;

			case 'x': {
				save_state_path = strdup(optarg);	/* Memory leak */
			} break;

			case 'h':
			case '?':
			default:
				usage(argv[0]);
				exit(1);
		}
	}
        /* Make start addr = load addr, unless explicitly set: */
        if (start_addr == (ADR)~0 /* Assume not set from cmdline */) {
                start_addr = load_addr;
        }
}

Config	*getConfig()
{
	return &config;
}
