	/*
	 * PowerPC head/vectors/startup for firmware
	 *
	 * Copyright 2016-2022 Matt Evans
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

#include "hw.h"

#define UART_OUTPUT 1
#define PAGE_UART	(UART_PHYS_ADDR >> 16)

#define SPR_DEBUG       1023
#define SPR_DC_INV_SET  1022
#define SPR_IC_INV_SET  1021

#define INT32(r, i)	lis	(r), (i)@ha ; \
			addi	(r), (r), (i)@l

	.org 0

#define r0	0
#define r1	1
#define r2	2
#define r3	3
#define r4	4
#define r5	5
#define r6	6
#define r7	7
#define r8	8
#define r9	9
#define r10	10
#define r11	11
#define r12	12
#define r13	13
#define r14	14
#define r15	15
#define r16	16
#define r17	17
#define r18	18
#define r19	19
#define r20	20
#define r21	21
#define r22	22
#define r23	23
#define r24	24
#define r25	25
#define r26	26
#define r27	27
#define r28	28
#define r29	29
#define r30	30
#define r31	31

	.section .vectors

	.org 0
	b	_start

	////////////////////////////////////////////////////////////////////////
	.org 0x100
	.globl _start
_start:
vec_reset:
	b	startup

	////////////////////////////////////////////////////////////////////////
	.org 0x200
vec_mc:
	li	r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x300
vec_dsi:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x400
vec_isi:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31


	////////////////////////////////////////////////////////////////////////
	.org 0x500
vec_ei:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x600
vec_align:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x700
vec_prog:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x800
vec_fpu:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0x900
vec_dec:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0xc00
vec_sc:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31

	////////////////////////////////////////////////////////////////////////
	.org 0xd00
vec_trace:
        li      r31, 0x00ff
        mtspr   SPR_DEBUG, r31


	.section .text.startup
        ////////////////////////////////////////////////////////////////////////
startup:
        // Banner
        INT32(r3, banner)
        bl      pstr

        // Init registers:
        li      r1, 0
        oris    r0, r1, 0xffff
        ori     r1, r1, 0xffff
        mr      r0, r1
        mr      r2, r0
        mr      r3, r0
        mr      r4, r0
        mr      r5, r0
        mr      r6, r0
        mr      r7, r0
        mr      r8, r0
        mr      r9, r0
        mr      r10, r0
        mr      r11, r0
        mr      r12, r0
        mr      r13, r0
        mr      r14, r0
        mr      r15, r0
        mr      r16, r0
        mr      r17, r0
        mr      r18, r0
        mr      r19, r0
        mr      r20, r0
        mr      r21, r0
        mr      r22, r0
        mr      r23, r0
        mr      r24, r0
        mr      r25, r0
        mr      r26, r0
        mr      r27, r0
        mr      r28, r0
        mr      r29, r0
        mr      r30, r0
        mr      r31, r0

        li      r0, 0
        mtcr    r0
        mtxer   r0
        li      r0, 0x7fff
        ori     r0, r0, 0xffff
        mtdec   r0
        /* FIXME:
         * SPRGs, SRRs etc.
         */

	bl	cpu_dc_inval_all
	bl 	cpu_ic_inval_all

        tlbia
        sync

	// Zero BSS
	INT32(r2, _sbss)
	INT32(r3, _ebss)

	li	r4, 0
1:	stw	r4, 0(r2)
	addi	r2, r2, 4
	cmpw	r2, r3
	blt	1b

        INT32(r1, _stack_top)

	bl	main

1:	b	1b


	////////////////////////////////////////////////////////////////////////
        // Utilities
pstr:   // r3 = string
1:      lbz     r4, 0(r3)
        cmpwi   cr0, r4, 0
        beq     cr0, 2f
#ifdef UART_OUTPUT
	lis	r5, PAGE_UART
	// Wait for TX non-full!
3:	lbz	r6, 4(r5)
	andi.	r6, r6, 2
	beq	cr0, 3b
	stb	r4, 0(r5)	// UART TX
#else // Debug SPR console
        ori     r4, r4, 0x100   // Printout
        mtspr   SPR_DEBUG, r4
#endif
        addi    r3, r3, 1
        b       1b
2:      blr


putch:
#ifdef UART_OUTPUT
	lis	r4, PAGE_UART
	// Wait for TX non-full!
3:	lbz	r5, 4(r4)
	andi.	r5, r5, 2
	beq	cr0, 3b
	stb	r3, 0(r4)	// UART TX
#else // Debug SPR console
        ori     r3, r3, 0x100
        mtspr   SPR_DEBUG, r3
#endif
        blr


phex32: mfctr   r5
        li      r4, 8
        mtctr   r4
1:      rlwinm  r3, r3, 4, 0,31 // ROR#4
        andi.   r4, r3, 0xf
        cmpwi   cr0, r4, 10
        bge     cr0,2f
        addi    r4, r4, '0'
        b       3f
2:      addi    r4, r4, 'a'-10
3:
#ifdef UART_OUTPUT
	lis	r7, PAGE_UART
	// Wait for TX non-full!
4:	lbz	r6, 4(r7)
	andi.	r6, r6, 2
	beq	cr0, 4b
	stb	r4, 0(r7)	// UART TX
#else // Debug SPR console
        ori     r4, r4, 0x100
        mtspr   SPR_DEBUG, r4
#endif
        bdnz    1b
        mtctr   r5
        blr


	// Invalidate whole D$ using MR's IMPDEF set-inval
cpu_dc_inval_all:
	li	r3, CPU_CACHE_SETS
	mtctr	r3
	li	r4, 0
1:	mtspr	SPR_DC_INV_SET, r4
	addi	r4, r4, CPU_CACHE_CL
	bdnz	1b
	blr

	// Same, invalidate whole I$
cpu_ic_inval_all:
	li	r3, CPU_CACHE_SETS
	mtctr	r3
	li	r4, 0
1:	mtspr	SPR_IC_INV_SET, r4
	addi	r4, r4, CPU_CACHE_CL
	bdnz	1b
	blr


	.section .rodata

banner:
#ifdef UART_OUTPUT
        .asciz "*** Booting (UART) ***\n"
#else
        .asciz "*** Booting ***\n"
#endif

	.align 8

        .end
