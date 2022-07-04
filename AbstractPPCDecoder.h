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

#ifndef PPCDECODER_H
#define PPCDECODER_H

#include "types.h"
#include "PPCInstructionFields.h"

/* Take an instr. and call a method on a derived class.
 */

typedef void * RET_TYPE;

template<class T>
class AbstractPPCDecoder
{
public:
        AbstractPPCDecoder() {};

	void	*decode(u32 inst)
	{
		unsigned int opcode = getOpcode(inst);

		/* Big switch, invokes methods in derived class: */
#include "AbstractPPCDecoder_decoder.h"
		return static_cast<T*>(this)->op_unk(inst);
	}

private:
	/* Field utility accessors for decode table above: */
#include "PPCInstructionFields.h"
};

#endif
