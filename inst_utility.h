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

/* Utility functions for arithmetic, logical ops, compare, etc.
 */

#ifndef INST_UTILITY_H
#define INST_UTILITY_H

/* Arithmetic 'dot' instructions set CR0 with a comparison to 0: */
#define SET_CR0(x) 		do {		  \
		u32 cr = READ_CR();		  \
		WRITE_CR_FIELD(cr, 0, CMP(x, 0)); \
		WRITE_CR(cr);			  \
	} while(0)

/* Set XER.OV and OR the flag into XER.SO: */
#define SET_SO_OV(x)		do {				\
		cpus->setXER_SO((x) || cpus->getXER_SO());	\
		cpus->setXER_OV(x);				\
	} while(0)

/* Macros for cleaner calculation of carry and overflow:
 *
 * Carry-out isn't that nice to calculate, but we can do this by
 * treating the operands as two 32-bit unsigned values adding into a
 * 64-bit result, then look at bit 32.
 *
 * Overflow could be calculated as described in:
 *  http://teaching.idallen.com/cst8214/08w/notes/overflow.txt
 *
 * i.e., look at the sign of operands and the result's sign.
 * *However*, if we're carrying in a bit then that's (in C) a second
 * addition and that might itself overflow!
 *
 * One approach might be to do a 31-bit add (i.e. watch for the
 * carry-out from bit 30 into 31 for the requested addition *plus* the
 * carry-in) and then manually add bits 31, grabbing the carry-out
 * into bit 32 (as above) for the overflow.  The first carry-out is
 * the carry-in into the top bit, and the second is the carry-out, so
 * we can calculate overall overflow by in != out.  This is what
 * hardware would do, but is super-fiddly in C.
 *
 * So, use the sign-checking technique and include carry-in; we
 * get the following truth tables:
 *
 *   SA SB SR  OV
 *   0  0  0   0
 *   0  0  1   1 * (positive+positive ending up negative)
 *   0  1  0   0
 *   0  1  1   0
 *   1  0  0   0
 *   1  0  1   0
 *   1  1  0   1 * (negative+negative ending up positive)
 *   1  1  1   0
 *
 * Treating carry-in as a second add (with positive sign), overflow is if * above, or if:
 *   SR SC SR  OV2
 *   0  0  0   0
 *   0  0  1   1 * (positive+positive ending up negative)
 *   1  0  0   0
 *   1  0  1   0
 *
 * TODO: Investigate whether inline asm (on hosts that support
 * carry/OV flags) makes a useful difference.
 *
 */
#define ADD_OV_CO_CI(val, ov, co, a, b, ci)	do {			\
	REG ia = (a);							\
	REG ib = (b);							\
	REG ic = (ci);							\
	u64 r = (u64)ia + (u32)ib + (u32)ic;				\
	co = (r & 0x100000000) ? 1 : 0;					\
	val = r & 0xffffffff;						\
	/* Calc OVF */							\
	u32 ra = (u32)r;						\
	ov = ( !(ia & 0x80000000) && !(ib & 0x80000000) &&  (ra & 0x80000000) ) || \
		(  (ia & 0x80000000) &&  (ib & 0x80000000) && !(ra & 0x80000000) ); \
	} while(0)

#define ADD_CO(val, co, a, b)			do { u32 d; ADD_OV_CO_CI(val, d, co, a, b, 0); } while(0)
#define ADD_OV(val, ov, a, b)			do { u32 d; ADD_OV_CO_CI(val, ov, d, a, b, 0); } while(0)
#define ADD_CO_CI(val, co, a, b, ci)		do { u32 d; ADD_OV_CO_CI(val, d, co, a, b, ci); } while(0)
#define ADD_OV_CI(val, ov, a, b, ci)		do { u32 d; ADD_OV_CO_CI(val, ov, d, a, b, ci); } while(0)
#define ADD_OV_CO(val, ov, co, a, b)		do {        ADD_OV_CO_CI(val, ov, co, a, b, 0); } while(0)

/* Return a CR-style 4-bit result of comparing x & y; note bit 0 is MSB of this nybble:
 * bit 0:  LT
 * bit 1:  GT
 * bit 2:  EQ
 * bit 3:  SO (set from XER.SO)
 */
#define CMP(x,y)		( ( (sREG)(x) < (sREG)(y) ? 8 :		\
				    ((sREG)(x) > (sREG)(y) ? 4 : 2 ) ) | cpus->getXER_SO() )
#define CMPu(x,y)		( ( (REG)(x) < (REG)(y) ? 8 :		\
				    ((REG)(x) > (REG)(y) ? 4 : 2 ) ) | cpus->getXER_SO() )

/* Read bit in position y of a word; beware bit number is IBM-order! */
#define RBIT(x,y)		(((x) >> (31-(y))) & 1)
/* Insert bit z into position y of word x; beware bit number is IBM-order! */
#define WBIT(x,y,z)		do {					\
		(x) = ((x) & ~(1 << (31-(y)))) | (((z) & 1) << (31-(y)));	\
	} while(0)

/* Write the 4-bit field at position y, in x, with value z; field 0 is at MSB */
#define WRITE_CR_FIELD(x,y,z)	do {					\
		(x) = ((x) & ~(0xf << ((7-(y))*4))) | ((z) << ((7-(y))*4)); \
	} while(0)

/* Read the 4-bit field at position y in x, field 0 being at MSB */
#define READ_CR_FIELD(x,y)	(((x) >> ((7-(y))*4)) & 0xf)

#define SXT8(x)			((REG)((int8_t)(x)))
#define SXT16(x)		((REG)((int16_t)(x)))

#define BSWAP16(x)		((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define BSWAP32(x)		((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | \
				 (((x) << 8) & 0xff0000) | (((x) << 24) & 0xff000000))

#define ROTL32(x, s)            ((((x) << (s)) & 0xffffffff) | (((x) >> (32-(s))) & 0xffffffff))

#define SIGN(x)			((uint32_t)(x) >> 31)

#endif
