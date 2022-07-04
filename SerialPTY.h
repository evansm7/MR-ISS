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

#ifndef SERIALPTY_H
#define SERIALPTY_H

#include <pthread.h>
#include "types.h"
#include "AbstractSerial.h"

class SerialPTY : public AbstractSerial
{
	////////////////////////////////////////////////////////////////////////////////
public:
        SerialPTY(bool wait_connect = false)
		: tty_fd(-1), rx_cb(0), reportRX(true)
	{} /* Todo, wait for connect */

	void	init();

	void 	putByte(u8 b);
	u8 	getByte();
	bool 	rxPending();
	// Register callback for new data (callback may be called from another thread!)
	void 	registerAsyncRxCB(void (*func)(void *), void *arg);
	// TODO: Could also do state-changing callback (e.g connect/disconnect)

	// Don't call this...
	void	listenThread();

private:
	bool	initTTY();
	void	initListenerThread();

	int	tty_fd;
	pthread_t	pt_listener;
	void	(*rx_cb)(void *arg);
	void	*rx_cb_arg;
	bool	reportRX;
};

#endif
