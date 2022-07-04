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

/* Simple managemnt interface via socket
 *
 * Talks to human, or to a script for logging/display purposes.
 *
 * Matt Evans, 22 Sept 2018
 */
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "management.h"
#include "log.h"
#include "stats.h"
#include "PPCCPUState.h"


static 	int skt_fd = -1;
static  int listen_fd = -1;
#define BUF_SZ 4096
static unsigned int current_input_pos = 0;
static char	input_buffer[BUF_SZ];
static char	output_buffer[BUF_SZ];

/* Stuff from elsewhere ... */
extern PPCCPUState	pcs;	// UP for now.

/******************************************************************************/

static	void	*mgmt_thread(void *arg);
static 	void	get_input();
static 	pthread_t	mgmt_listener;
static  char 	*skip_whitespace(char *p);
static  char 	*find_whitespace(char *p);

/******************************************************************************/

static	void	write_response(char *response)
{
	int len = strlen(response);
	int p;

	if (skt_fd == -1)
		return;

	for (p = 0; p < len; ) {
		int r = write(skt_fd, &response[p], len-p);
		if (r < 0) {
			perror("management: write response error");
			return;
		}
		p += r;
	}
}

/* Management operations */

static	void	stats_send()
{
	size_t s = BUF_SZ;
	int r;
	char *p = output_buffer;
	for (int i = COUNTER_START+1; i < COUNTER_MAX; i++) {
		r = snprintf(p, s, "%s %lld, ",
			     global_counter_names[i-1],
			     global_counters[i]);
		if (r < 0)
			return;
		s -= r;
		p += r;
	}
	if (s <= 1) {
		p = &output_buffer[BUF_SZ-2];
	}
	*p++ = '\n';
	*p = '\0';

	write_response(output_buffer);
}

static 	void	cpu_state_send(int n)
{
	/* FIXME: Only UP/CPU0 for now. */
	pcs.render_mr(output_buffer, BUF_SZ);
	write_response(output_buffer);
}

static	void	log_send()
{
	snprintf(output_buffer, BUF_SZ,
		 "LOG_FLAG_DBG %d, "
		 "LOG_FLAG_DISASS %d, "
		 "LOG_FLAG_STRACE %d, "
		 "LOG_FLAG_IO_TRACE %d, "
		 "LOG_FLAG_BRANCH_TRACE %d, "
		 "LOG_FLAG_MMU_TRACE %d, "
		 "LOG_FLAG_EXC_TRACE %d, "
		 "\n",
		 !!(log_enables_mask & LOG_FLAG_DBG),
		 !!(log_enables_mask & LOG_FLAG_DISASS),
		 !!(log_enables_mask & LOG_FLAG_STRACE),
		 !!(log_enables_mask & LOG_FLAG_IO_TRACE),
		 !!(log_enables_mask & LOG_FLAG_BRANCH_TRACE),
		 !!(log_enables_mask & LOG_FLAG_MMU_TRACE),
		 !!(log_enables_mask & LOG_FLAG_EXC_TRACE));

	/* TODO: Add CPU state dump flag */

	write_response(output_buffer);
}

#define TEST_FLAG(x) 					\
	if (!strcasecmp(flag, #x)) { 			\
		if (val[0] == '1')			\
			log_enables_mask |= (x); 	\
		else					\
			log_enables_mask &= ~(x);	\
	}

static	void	log_set(char *flag, char *val)
{
	DEBUG("log_set: %s, %s\n", flag, val);

	TEST_FLAG(LOG_FLAG_DBG)
	else
	TEST_FLAG(LOG_FLAG_DISASS)
	else
	TEST_FLAG(LOG_FLAG_STRACE)
	else
	TEST_FLAG(LOG_FLAG_IO_TRACE)
	else
	TEST_FLAG(LOG_FLAG_BRANCH_TRACE)
	else
	TEST_FLAG(LOG_FLAG_MMU_TRACE)
	else
	TEST_FLAG(LOG_FLAG_EXC_TRACE)
}

static	void	process_line(char *line)
{
	DEBUG("Management, read line: [%s]\n", line);

	/* The dumbest parser evar! */
	if (!strcasecmp(line, "help")) {
		write_response((char *)"STATS\n"
			       "RESET-STATS\n"
			       "CPU<n>\n"
			       "GETLOG\n"
			       "SETLOG <flagnr> <0|1>\n"
			);
	} else if (!strcasecmp(line, "STATS")) {
		stats_send();
	} else if (!strcasecmp(line, "RESET-STATS")) {
		stats_reset();
	} else if (!strncasecmp(line, "CPU", 3)) {
		/* line[3] is CPU number */
		cpu_state_send(0); /* FIXME: CPU number parse */
	} else if (!strcasecmp(line, "GETLOG")) {
		/* Get logging flag states */
		log_send();
	} else if (!strncasecmp(line, "SETLOG", 6)) {
		/* Get logging flag states */
		/* Syntax: SETLOG <flagnr> <0|1> */
		char *flag = skip_whitespace(find_whitespace(line));
		char *end_flag = find_whitespace(flag);
		char *val = skip_whitespace(end_flag);
		*end_flag = '\0';
		log_set(flag, val);
	} else {
		write_response((char *)"ONOES\n");
	}
}

/******************************************************************************/

void	management_init()
{
	pthread_attr_t	attr;
        struct sockaddr_in listenaddr;

	LOG("Opening management socket... ");

        if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("Can't create listening socket\n");
                return;
        }

        /* Bind the local address to the sending socket, any port number */
        listenaddr.sin_family = AF_INET;
        listenaddr.sin_addr.s_addr = INADDR_ANY;
        listenaddr.sin_port = htons(MGMT_PORT);

	int so_reuseaddr = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

        if (bind(listen_fd, (struct sockaddr *)&listenaddr, sizeof(listenaddr))) {
                perror("Can't bind() socket\n");
                return;
        }
	if (listen(listen_fd, 1)) {
		perror("Can't listen() on socket\n");
		return;
	}

	LOG("Done, listening on port %d.\n", MGMT_PORT);

	LOG("Starting management thread... ");
	pthread_attr_init(&attr);
	if (pthread_create(&mgmt_listener, &attr, mgmt_thread, NULL)) {
		perror("pthread_create:");
	} else {
		LOG("Done.\n");
	}
}

static	void	*mgmt_thread(void *arg)
{
	DEBUG("Management thread starting.\n");

	while (1) {
		int num = 1;
		struct pollfd f[] = {
			[0] = {
			.fd = listen_fd,
			.events = POLLIN,
			.revents = 0
			},
			[1] = {
			.fd = skt_fd,
			.events = POLLIN,
			.revents = 0
			}
		};

		if (skt_fd != -1)
			num = 2;

		if (poll(f, num, -1) > 0) {
			if (f[0].revents != 0) {
				/* New connection */
				struct sockaddr_in addr;
				socklen_t alen = sizeof(addr);

				skt_fd = accept(listen_fd, (struct sockaddr *)&addr, &alen);
				if (skt_fd >= 0) {
					if (fcntl(skt_fd, F_SETFL, O_NONBLOCK) == -1) {
						perror("Can't set socket non-blocking\n");
					}
				}
			}

			if (f[1].revents & POLLERR || f[1].revents & POLLHUP) {
				DEBUG("Management: Connection dropped on fd %d\n", skt_fd);
				skt_fd = -1;
			} else if (f[1].revents & POLLIN) {
				/* Activity on open connection */
				get_input();
			}
		}
        }
}

/* Input pending on skt_fd.
 * To make this as trivial as possible, just read into a huge buffer...
 */
static 	void	get_input()
{
	while (current_input_pos < BUF_SZ-1) {
		int r = read(skt_fd, &input_buffer[current_input_pos], BUF_SZ-current_input_pos-1);
		if (r < 1) {
			/* Since we're non-blocking, might get EAGAIN. */
			if (r < 0 && errno != EAGAIN) {
				perror("management read");
				close(skt_fd);
				DEBUG("Management: Connection dropped on fd %d\n", skt_fd);
				skt_fd = -1;
			}
			/* But if r==0/EAGAIN, there's just nothing else to do. */
			goto out;
		}

		current_input_pos += r;

		/* Search for newline */
		int start_pt = 0;
		for (int i = 0; i < current_input_pos; i++) {
			if (input_buffer[i] == '\n') {
				/* Terminatez-ca: */
				input_buffer[i] = '\0';
				process_line(&input_buffer[start_pt]);
				start_pt = i + 1;
			}
		}
		/* If consumed a line, we might also have read part of the next line, so copy down to bottom: */
		if (start_pt < (current_input_pos - 1)) {
			/* Line processed from the previous start_pt to the
			 * current start_pt.  Since the current start_pt is
			 * before the end of the buffer, but the rest of the
			 * buffer does not contain a newline, shuffle the entire
			 * crapfest down to the bottom of the buffer for next
			 * time.
			 */
			memmove(&input_buffer[0], &input_buffer[start_pt], current_input_pos-start_pt);
			/* And then the next read() drops onto the end of this: */
			current_input_pos = current_input_pos-start_pt;
		} else {
			/* Newline was at end of buffer; nothing to save for the next read.  Reset everything.
			 */
			current_input_pos = 0;
		}
	}

out:
	return;
}

static  char 	*skip_whitespace(char *p)
{
	while (*p == ' ' && *p != '\0') { p++; }
	return p;
}

static  char 	*find_whitespace(char *p)
{
	while (*p != ' ' && *p != '\0') { p++; }
	return p;
}
