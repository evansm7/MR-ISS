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

#ifndef PPC_INSTRUCTION_FIELDS_H
#define PPC_INSTRUCTION_FIELDS_H

static __attribute__((unused)) unsigned int 	getOpcode(u32 i)		{ return (i >> 26) & 0x3f; }
static __attribute__((unused)) unsigned int 	X_XOPC(u32 i)			{ return (i >> 1) & 0x3ff; }
static __attribute__((unused)) unsigned int 	XL_XOPC(u32 i)			{ return (i >> 1) & 0x3ff; }

static __attribute__((unused)) unsigned int 	getLI(u32 i)			{ return (i >> 2) & 0xffffff; }

static __attribute__((unused)) unsigned int 	I_LI(u32 i)			{ return i & 0x03fffffc; }	/* NB: Not shifted! */
static __attribute__((unused)) unsigned int 	I_AA(u32 i)			{ return (i >> 1) & 0x1; }
static __attribute__((unused)) unsigned int 	I_LK(u32 i)			{ return i & 0x1; }

static __attribute__((unused)) unsigned int 	B_BO(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	B_BI(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	B_BD(u32 i)			{ return i & 0xfffc; }  	/* NB: Not shifted! */
static __attribute__((unused)) unsigned int 	B_AA(u32 i)			{ return (i >> 1) & 0x1; }
static __attribute__((unused)) unsigned int 	B_LK(u32 i)			{ return i & 0x1; }

static __attribute__((unused)) unsigned int 	SC_LEV(u32 i)			{ return (i >> 5) & 0x7f; }
static __attribute__((unused)) unsigned int 	SC_XOPC(u32 i)			{ return (i >> 1) & 1; }

static __attribute__((unused)) unsigned int 	D_RA(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	D_RA0(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	D_RT(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	D_RS(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	D_TO(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) s32		D_SI(u32 i)			{ return (s16)(i & 0xffff); }
static __attribute__((unused)) u32		D_UI(u32 i)			{ return (u16)(i & 0xffff); }
static __attribute__((unused)) s32		D_D(u32 i)			{ return (s16)(i & 0xffff); }
static __attribute__((unused)) unsigned int 	D_BF(u32 i)			{ return (i >> 23) & 0x7; }
static __attribute__((unused)) unsigned int 	D_L(u32 i)			{ return (i >> 21) & 1; }

static __attribute__((unused)) unsigned int 	X_BF(u32 i)			{ return (i >> 23) & 7; }
static __attribute__((unused)) unsigned int 	X_RT(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_RS(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_TO(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_L(u32 i)			{ return (i >> 21) & 3; }	/* Sync; beware, there are multiple L fields in X */
static __attribute__((unused)) unsigned int 	X_RA(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_RA0(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_E(u32 i)			{ return (i >> 16) & 0xf; } 	/* Sync; beware, there are two E fields in X ;( */
static __attribute__((unused)) unsigned int 	X_RB(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_NB(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_SH(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_EH(u32 i)			{ return i & 1; }
static __attribute__((unused)) unsigned int 	X_Rc(u32 i)			{ return i & 1; }
static __attribute__((unused)) unsigned int 	X_TH(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	X_CT(u32 i)			{ return (i >> 21) & 0xf; }
static __attribute__((unused)) unsigned int 	X_SR(u32 i)			{ return (i >> 16) & 0xf; }

static __attribute__((unused)) unsigned int 	XL_BT(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	XL_BO(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	XL_BA(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	XL_BI(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	XL_BH(u32 i)			{ return (i >> 11) & 0x3; }
static __attribute__((unused)) unsigned int 	XL_LK(u32 i)			{ return i & 1; }
static __attribute__((unused)) unsigned int 	XL_BB(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	XL_BF(u32 i)			{ return (i >> 23) & 0x7; }
static __attribute__((unused)) unsigned int 	XL_BFA(u32 i)			{ return (i >> 18) & 0x7; }

static __attribute__((unused)) unsigned int 	XFX_RT(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	XFX_RS(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	XFX_spr(u32 i)			{ return ((i >> (11+5)) & 0x1f) | ((i >> (11-5)) & 0x3e0); }
static __attribute__((unused)) unsigned int 	XFX_FXM(u32 i)			{ return (i >> 12) & 0xff; }

static __attribute__((unused)) unsigned int 	XO_RT(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	XO_RA(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	XO_RB(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	XO_OE(u32 i)			{ return (i >> 10) & 1; }	/* Don't need to mask, used as bool */
static __attribute__((unused)) unsigned int 	XO_Rc(u32 i)			{ return i & 1; }

static __attribute__((unused)) unsigned int 	M_RA(u32 i)			{ return (i >> 16) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_RB(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_RS(u32 i)			{ return (i >> 21) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_SH(u32 i)			{ return (i >> 11) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_MB(u32 i)			{ return (i >> 6) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_ME(u32 i)			{ return (i >> 1) & 0x1f; }
static __attribute__((unused)) unsigned int 	M_Rc(u32 i)			{ return i & 1; }

#endif
