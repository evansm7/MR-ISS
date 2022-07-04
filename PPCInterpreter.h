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

#ifndef PPCINTERP_H
#define PPCINTERP_H

#include <cstddef>

#include "AbstractPPCDecoder.h"
#include "PPCCPUState.h"
#include "PPCMMU.h"
#include "stats.h"
#include "inst_utility.h"


#define INTERP_RETURN	do { return 0; } while(0)  // Return value = ?
#define MFCHECK(RnW)								\
	do {									\
		if (mem_access_fault != PPCMMU::FAULT_NONE) {			\
			cpus->raiseMemException(RnW, false, mem_access_addr,	\
						mem_access_fault, inst);        \
			INTERP_RETURN; 						\
		}								\
	} while(0)

class PPCInterpreter : public AbstractPPCDecoder<PPCInterpreter>
{
public:
        PPCInterpreter() : want_break(false)
		,mem_access_fault(PPCMMU::FAULT_NONE)
		,exc_generation_count(0)
	{};

	void		execute()
	{
		void *ret;	// What's the return value for, in the interpreter?
	fetch:
		pc 		= cpus->getPC();
		PPCMMU::fault_t	fault = mmu->loadInst32(pc, &inst, cpus->isPrivileged());
		if (fault != PPCMMU::FAULT_NONE) {
                        cpus->raiseMemException(true, true, pc, fault, 0);
			goto fetch;
		}
		ret = decode(inst);
		(void)ret;
		COUNT(CTR_INST_EXECUTED);
	}

	void 		setCPUState(PPCCPUState *c)	{ cpus = c; }
	void 		setMMU(PPCMMU *m)		{ mmu = m; }

	bool		breakRequested()		{ return want_break; }
	void		breakRequest()			{ want_break = true; }

	static size_t   getCPUSoffset()
	{
		return (uint8_t *)&((PPCInterpreter *)0)->cpus - (uint8_t *)((PPCInterpreter *)0);
	}

        void            setPCInst(REG p, u32 i)         { pc = p; inst = i; }
#if DUMMY_MEM_ACCESS == 1
        u32             read_data;
#endif

	////////////////////////////////////////////////////////////////////////////////
	// Implementations of functions from AbstractPPCDecoder:
#include "PPCInterpreter_prototypes.h"

	////////////////////////////////////////////////////////////////////////////////

private:
	PPCCPUState 	*cpus;

	PPCMMU		*mmu; 	/* All memory accesses go through here */

	VA		pc;
	u32		inst;

	bool		want_break;

	/* A bit of a grim way of doing it, but this gets set to a fault value
	 * when a LOADx/STOREx helper is called.  This must be checked by an
	 * instr implementation function by calling MFCHECK() (which deals
	 * with returning cleanly from the impl function).
	 */
	PPCMMU::fault_t	mem_access_fault;
	VA		mem_access_addr;

	/* This is a horrible hack to get lwarx/stwcx to do something
	 * 'reasonable', to make a stwcx fail if an exception or RFI has
	 * occurred since the lwarx.  A safer but more heavyweight way to do
	 * this would be to actually check a reservation address (by PA...).
	 * Any MMU change also ups this.
	 */
	unsigned long	exc_generation_count;
	VA		reservation_address;	/* Should be a PA!!! */
	unsigned long	reservation_gencount;

	/************************* Register access ***********************/
	REG		READ_GPR_OZ(unsigned int roz)	{ return roz ? cpus->getGPR(roz) : 0; }
	REG		READ_GPR(unsigned int reg)	{ return cpus->getGPR(reg); }
	REG32		READ_CA()			{ return cpus->getXER_CA(); }
	REG32		READ_CR()			{ return cpus->getCR(); }
	bool		getCRb(unsigned int bit)	{ return !!((cpus->getCR() >> (31-bit)) & 1); }
	REG32		READ_MSR()			{ return cpus->getMSR(); }

	void		WRITE_GPR(unsigned int reg, REG val)	{ cpus->setGPR(reg, val); }
	void            WRITE_CA(bool ca)               { cpus->setXER_CA(ca); }
	void            WRITE_CR(REG32 val)             { cpus->setCR(val); }
	void            WRITE_MSR(REG32 val);

	void		incPC()				{ cpus->setPC(cpus->getPC() + 4); }
	REG		getPC()				{ return cpus->getPC(); }

	/************************* Memory access *************************/
#if DUMMY_MEM_ACCESS == 0
	REG		LOAD8(ADR addr)
	{
		u8	val;
		mem_access_fault = mmu->load8(addr, (u8 *)&val, cpus->isPrivileged());
		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
		return val;
	}

	REG		LOAD16(ADR addr)
	{
		u16	val;
		mem_access_fault = mmu->load16(addr, (u16 *)&val, cpus->isPrivileged());
		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
		return val;
	}

	REG		LOAD32(ADR addr)
	{
		u32	val;
		mem_access_fault = mmu->load32(addr, (u32 *)&val, cpus->isPrivileged());
		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
		return val;
	}

	void		STORE8(ADR addr, uint8_t val)
	{
		mem_access_fault = mmu->store8(addr, val, cpus->isPrivileged());

		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
	}

	void		STORE16(ADR addr, uint16_t val)
	{
		mem_access_fault = mmu->store16(addr, val, cpus->isPrivileged());

		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
	}

	void		STORE32(ADR addr, uint32_t val)
	{
		mem_access_fault = mmu->store32(addr, val, cpus->isPrivileged());

		if (mem_access_fault != PPCMMU::FAULT_NONE)
			mem_access_addr = addr;
	}
#else
	REG		LOAD8(ADR addr)
	{
		return read_data;
	}

	REG		LOAD16(ADR addr)
	{
		return read_data;
	}

	REG		LOAD32(ADR addr)
	{
		return read_data;
	}

	void		STORE8(ADR addr, uint8_t val)
	{
	}

	void		STORE16(ADR addr, uint16_t val)
	{
	}

	void		STORE32(ADR addr, uint32_t val)
	{
	}
#endif

	/************************* Miscellaneous *************************/
	void	       *UNDEFINED()			{ return op_unk(inst); }
	void	       	NOP()				{ }
	void	      	CHECK_YOUR_PRIVILEGE()		{ /* Todo: Raise program check */ }
	void	      	CHECK_YOUR_HYP_PRIVILEGE()	{ /* Todo: Raise program check */ }

        void            helper_DEBUG(REG val);

        void		SIM_QUIT(u32 returncode)
	{
		LOG("+++ EXIT request, value = %08x\n", returncode);
		want_break = true;
	}

	void		blow_reservation()		{ exc_generation_count++; }
};

#endif
