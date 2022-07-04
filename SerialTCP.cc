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

/* Super-simple TCP listener for basic console
 *
 * This is intended to have a human on the other end so isn't optimised for
 * performance.  It's a bit syscall-heavy.
 *
 * Matt Evans, 5 June 2018
 */
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include "SerialTCP.h"
#include "log.h"


void	SerialTCP::init()
{
	skt_fd = -1;

	initNet();
	/* Listener thread accepts connections & notifies of incoming data via
	 * CB:
	 */
	initListenerThread();
}

void	SerialTCP::initNet()
{
        struct sockaddr_in listenaddr;

	skt_fd = -1;

        if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("Can't create listening socket\n");
                return;
        }

        /* Bind the local address to the sending socket, any port number */
        listenaddr.sin_family = AF_INET;
        listenaddr.sin_addr.s_addr = INADDR_ANY;
        listenaddr.sin_port = htons(port);

        if (bind(listen_fd, (struct sockaddr *)&listenaddr, sizeof(listenaddr))) {
                perror("Can't bind() socket\n");
                return;
        }
	if (listen(listen_fd, 1)) {
		perror("Can't listen() on socket\n");
		return;
	}

	DEBUG("SerialTCP: Listening on port %d (fd %d)\n", port, listen_fd);
}

void	SerialTCP::listenThread()
{
	DEBUG("SerialTCP: Started RX thread\n");
	reportRX = true;

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
				DEBUG("SerialTCP: Connection dropped on fd %d\n", skt_fd);
				skt_fd = -1;
			} else if (f[1].revents & POLLIN) {
				// Some input!
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
	SerialTCP *c = (SerialTCP *)arg;
	c->listenThread();
	return 0;
}

void	SerialTCP::initListenerThread()
{
	pthread_attr_t	attr;

	DEBUG("SerialTCP: Starting RX thread\n");
	pthread_attr_init(&attr);
	if (pthread_create(&pt_listener, &attr, startThread, this)) {
		perror("pthread_create:");
	}
}

void 	SerialTCP::putByte(u8 b)
{
	if (skt_fd != -1)
		write(skt_fd, &b, 1);
}

u8 	SerialTCP::getByte()
{
	u8 b = 0;
	if (skt_fd != -1)
		read(skt_fd, &b, 1);
	reportRX = true;
	return b;
}

bool 	SerialTCP::rxPending()
{
	bool rx_ok;
	struct pollfd f = {
		.fd = skt_fd,
		.events = POLLIN,
		.revents = 0
	};

	if (skt_fd == -1)
		rx_ok = false;
	else
		rx_ok = (poll(&f, 1, 0) == 1);
	return rx_ok;
}

void 	SerialTCP::registerAsyncRxCB(void (*func)(void *), void *arg)
{
	rx_cb = func;
	rx_cb_arg = arg;
}
