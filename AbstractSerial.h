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

#ifndef ABSTRACT_SERIAL_H
#define ABSTRACT_SERIAL_H

#include "types.h"


class AbstractSerial
{
	////////////////////////////////////////////////////////////////////////////////
protected:
	AbstractSerial() {};

	////////////////////////////////////////////////////////////////////////////////
public:
	virtual void	init() = 0;
	virtual void 	putByte(u8 b) = 0;
	virtual u8 	getByte() = 0;
	virtual bool 	rxPending() = 0;
	// Register callback for new data (callback may be called from another thread!)
	virtual void 	registerAsyncRxCB(void (*func)(void *), void *arg) = 0;
};

#endif
