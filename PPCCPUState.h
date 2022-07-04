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

#ifndef PPCCPUSTATE_H
#define PPCCPUSTATE_H

#include <setjmp.h>
#include <cstddef>

#include "types.h"
#include "utility.h"
#include "PPCMMU.h"

#define RESET_MSR_VAL	0x0

/* TB increments by 1 every 4 instructions: */
#define TB_SHIFT	2

class PPCCPUState {
public:
        PPCCPUState();

	////////////////////////////////////////////////////////////////////////////////
	// Types
	typedef enum {
		EXC_RESET = 0x100,
		EXC_MC = 0x200,
		EXC_DSI = 0x300,
		EXC_DS = 0x380,		/* 64-bit! */
		EXC_ISI = 0x400,
		EXC_IS = 0x480,		/* 64-bit! */
		EXC_IRQ = 0x500,
		EXC_ALIGN = 0x600,
		EXC_PROG = 0x700,
		EXC_FPUN = 0x800,
		EXC_DEC = 0x900,
		EXC_SC = 0xc00,
		EXC_TRACE = 0xd00,
		EXC_PMU = 0xf00
	} exception_t;

	////////////////////////////////////////////////////////////////////////////////
	// Methods
	void	dump();
	void	render_mr(char *buffer, size_t len);

	void	setMMU(PPCMMU *m)			{ mmu = m; }
	PPCMMU *getMMU()				{ return mmu; }
	void	setJmpbuf(jmp_buf *j)			{ exception_jmp = j; }

	// Set accessors
	void	setGPR(unsigned int r, REG d)		{ ASSERT(r < 32); gprs[r] = d; }
	void	setPC(REG v)				{ pc = v; }
	void	setCTR(REG v)				{ ctr = v; }
	void	setLR(REG v)				{ lr = v; }
	void	setXER(REG32 v)				{ xer = v & 0xe000007f; }
	void    setXER_SO(bool v)			{ xer = (xer & ~XER_SO); if (v) { xer |= XER_SO; } }
	void    setXER_OV(bool v)			{ xer = (xer & ~XER_OV); if (v) { xer |= XER_OV; } }
	void    setXER_CA(bool v)			{ xer = (xer & ~XER_CA); if (v) { xer |= XER_CA; } }
	void	setCR(REG32 v)				{ cr = v; }
	void	setMSR(REG32 v)				{ msr = v; }
	void	setPIR(REG32 v)				{ pir = v; }
	void	setTB(u64 t)				{ tb = t << TB_SHIFT; }
	void	setHID0(u32 v)				{ hid0 = v; }
	void	setHID1(u32 v)				{ hid1 = v; }
	void	setSPRG0(REG v)				{ sprg0 = v; }
	void	setSPRG1(REG v)				{ sprg1 = v; }
	void	setSPRG2(REG v)				{ sprg2 = v; }
	void	setSPRG3(REG v)				{ sprg3 = v; }
	void	setSDR1(REG v)				{ sdr1 = v; }
	void	setSRR0(REG v)				{ srr0 = v; }
	void	setSRR1(REG v)				{ srr1 = v; }
	void	setDAR(REG v)				{ dar = v; }
	void	setDSISR(REG v)				{ dsisr = v; }
	void	setDEC(REG v)				{ dec = v; }

	// Get accessors
       	REG	getPC()					{ return pc; }
	REG32	getMSR()				{ return msr; }
	REG	getGPR(unsigned int r)			{ ASSERT(r < 32); return gprs[r]; }
	REG	getCTR()				{ return ctr; }
	REG	getLR()					{ return lr; }
	REG32	getXER()				{ return xer; }
	u32	getXER_SO()				{ return (xer & XER_SO) ? 1 : 0; }
	u32	getXER_OV()				{ return (xer & XER_OV) ? 1 : 0; }
	u32	getXER_CA()				{ return (xer & XER_CA) ? 1 : 0; }
	REG32	getCR()					{ return cr; }
	u32	getPIR()				{ return pir; }
	u64	getTB()					{ return (tb >> TB_SHIFT); }
	u32	getHID0()				{ return hid0; }
	u32	getHID1()				{ return hid1; }
	REG	getSPRG0()				{ return sprg0; }
	REG	getSPRG1()				{ return sprg1; }
	REG	getSPRG2()				{ return sprg2; }
	REG	getSPRG3()				{ return sprg3; }
	REG	getSDR1()				{ return sdr1; }
	REG	getSRR0()				{ return srr0; }
	REG	getSRR1()				{ return srr1; }
	REG	getDAR()				{ return dar; }
	REG	getDSISR()				{ return dsisr; }
	REG	getDEC()				{ return dec; }

	bool	isPrivileged()				{ return !(msr & MSR_PR); }
	bool	isHyp()					{ return false; }

	// Misc
	void	CPUTick(unsigned int t = 1)
	{
		cpu_inst_count += t;
		tb += t;
		if ((tb & ((1 << TB_SHIFT) - 1)) == 0)
			dec--;
	}

	u64	getCPUTicks()				{ return cpu_inst_count; }

	void	assertIRQ(bool status = true)		{ irqFlag = status; }

	bool	isDecrementerPending()
	{
		return (msr & MSR_EE) && (dec & 0x80000000);
	}

	bool	isIRQPending()
	{
		return (msr & MSR_EE) && irqFlag;
	}

	bool	getRawIRQPending()
	{
		return irqFlag;
	}

	/* Exceptions */
	void	raiseIRQException();
	void	raiseDECException();
	void	raisePROGException(u32 reason);
	void	raiseSCException();
        void	raiseMemException(bool RnW, bool InD, VA addr, PPCMMU::fault_t fault, u32 inst);

	static size_t    getPCoffset()
	{
		return (uint8_t *)&((PPCCPUState *)0)->pc - (uint8_t *)((PPCCPUState *)0);
	}

private:
	// Other objects:
	PPCMMU *mmu;
	jmp_buf *exception_jmp;

	// Private functions:
	void	takeException(exception_t e, REG new_srr1);

	// User state:
	REG	gprs[32];
	// No FPRs
	REG	pc;
	REG	ctr;
	REG	lr;
	REG32	xer;		// Is this ever 32?
	REG32	cr;

	// OS state:
	REG32	msr;
	REG	sprg0;
	REG	sprg1;
	REG	sprg2;
	REG	sprg3;
	REG	srr0;
	REG	srr1;
	REG	dar;
	REG	dsisr;
	REG	dec;

	REG	sdr1;

	// Implementation-defined internal state
	REG32	hid0;
	REG32	hid1;

	bool	irqFlag;

	// CPU number
	REG32	pir;

	u64	tb;

	// Sim state
	u64	cpu_inst_count;
};

#endif
