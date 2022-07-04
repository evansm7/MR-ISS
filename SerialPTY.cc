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

/* Super-simple pseudotty listener for basic console
 *
 * This is intended to have a human on the other end so isn't optimised for
 * performance.  It's a bit syscall-heavy.
 *
 * Matt Evans, 27 Aug 2018
 */
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <errno.h>
#if __DARWIN_UNIX03
#include <util.h>
#else
#include <pty.h>
#endif

#include "SerialPTY.h"
#include "log.h"

void	SerialPTY::init()
{
	tty_fd = -1;

	if (initTTY()) {
		/* Listener thread notifies of incoming data via CB:
		 */
		initListenerThread();
	} else {
		FATAL("Can't initialise SerialPTY, exiting.\n");
	}
}

bool	SerialPTY::initTTY()
{
	struct termios tos;
	char ptsn[PATH_MAX];
	int sfd;
	if (openpty(&tty_fd, &sfd, ptsn, &tos, NULL) < 0) {
		perror("SerialPTY: openpty");
		return false;
	}

	if (tcgetattr(sfd, &tos) < 0) {
		perror("Can't tcgetattr:");
		return false;
	}
	cfmakeraw(&tos);
	if (tcsetattr(sfd, TCSAFLUSH, &tos)) {
		perror("Can't tcsetattr:");
		return false;
	}
	close(sfd);

	int f = fcntl(tty_fd, F_GETFL);
	fcntl(tty_fd, F_SETFL, f | O_NONBLOCK);

	printf("SerialPTY:  FD %d connected to %s\n", tty_fd, ptsn);

	DEBUG("SerialPTY: Using fd %d\n", tty_fd);
	return true;
}

void	SerialPTY::listenThread()
{
	DEBUG("SerialPTY: Started RX thread\n");
	reportRX = true;

	while (1) {
		int num = 1;
		struct pollfd f[] = {
			[0] = {
			.fd = tty_fd,
			.events = POLLIN,
			.revents = 0
			}
		};

		if (poll(f, num, -1) > 0) {
			if (f[0].revents & POLLERR || f[0].revents & POLLHUP) {
				// ARGH FFS this will spin and spin before end is connected!  OS X bug?
				sleep(1);
			} else if (f[0].revents & POLLIN) {
				/* Input is present.
				 * NB: If UART isn't read then this will spin
				 * forever. :(
				 * FIXME: It would be better to read any pending
				 * data into a local queue, read via the UART
				 * regs.
				 */
				if (reportRX && rx_cb) {
					reportRX = false;
					rx_cb(rx_cb_arg);
				}
			}
		}
        }
}

static	void	*startThread(void *arg)
{
	SerialPTY *c = (SerialPTY *)arg;
	c->listenThread();
	return 0;
}

void	SerialPTY::initListenerThread()
{
	pthread_attr_t	attr;

	DEBUG("SerialPTY: Starting RX thread\n");
	pthread_attr_init(&attr);
	if (pthread_create(&pt_listener, &attr, startThread, this)) {
		perror("pthread_create:");
	}
}

void 	SerialPTY::putByte(u8 b)
{
	if (tty_fd != -1)
		write(tty_fd, &b, 1);
}

u8 	SerialPTY::getByte()
{
	u8 b = 0;
	if (tty_fd != -1) {
		int r;
		do {
			r = read(tty_fd, &b, 1);
		} while (r == -1 && errno == EAGAIN);
	}
	// FIXME:  up semaphore, rx thread is sleeping on that.  It'll then unblock callback again.
	reportRX = true;
	return b;
}

bool 	SerialPTY::rxPending()
{
	bool rx_ok;
	struct pollfd f = {
		.fd = tty_fd,
		.events = POLLIN,
		.revents = 0
	};

	if (tty_fd == -1)
		rx_ok = false;
	else
		rx_ok = (poll(&f, 1, 0) == 1);
	return rx_ok;
}

void 	SerialPTY::registerAsyncRxCB(void (*func)(void *), void *arg)
{
	rx_cb = func;
	rx_cb_arg = arg;
}
