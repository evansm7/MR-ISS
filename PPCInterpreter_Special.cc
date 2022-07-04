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

#include <stdio.h>
#include "PPCInterpreter.h"
#include "Config.h"


/********************* Special *********************/

RET_TYPE PPCInterpreter::op_eieio()
{
	DISASS("%08x   %08x  eieio\t  \n", pc, inst);

	/* Barrier:  For the purposes of this ISS, this does nothing. */

	incPC();
	COUNT(CTR_INST_EIEIO);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_isync()
{
	DISASS("%08x   %08x  isync\t  \n", pc, inst);

	/* Barrier: For the purposes of this ISS, this does nothing.
	 * In future, there *might* be a reason to implement this with
	 * respect to self-modifying code and JIT stuff but, for now,
	 * no reason.
	 */

	incPC();
	COUNT(CTR_INST_ISYNC);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mfspr(int spr, int RT)
{
	// Inputs 'spr', implicit inputs ''
	REG val_RT = 0;

	/* RT=SPR[spr] */
	switch (spr) {
		/* UISA */
	case SPR_LR:
		val_RT = cpus->getLR();
		DISASS("%08x   %08x  mflr\t r%d\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_CTR:
		val_RT = cpus->getCTR();
		DISASS("%08x   %08x  mfctr\t r%d\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_XER:
		val_RT = cpus->getXER();
		DISASS("%08x   %08x  mfxer\t r%d\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_PVR:
		val_RT = MR_PVR;
		DISASS("%08x   %08x  mfspr\t r%d, PVR\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_TBU:
		val_RT = cpus->getTB() >> 32;
		DISASS("%08x   %08x  mfspr\t r%d, TBU\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_TB:
		val_RT = cpus->getTB() & 0xffffffff;
		DISASS("%08x   %08x  mfspr\t r%d, TB\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_SPRG3:
		val_RT = cpus->getSPRG3();
		DISASS("%08x   %08x  mfspr\t r%d, SPRG3\t[%08x]\n", pc, inst, RT, val_RT);
		break;

		/* OEA privileged */
	case SPR_PIR:
		val_RT = cpus->getPIR();
		DISASS("%08x   %08x  mfspr\t r%d, PIR\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_SPRG0:
		val_RT = cpus->getSPRG0();
		DISASS("%08x   %08x  mfspr\t r%d, SPRG2\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_SPRG1:
		val_RT = cpus->getSPRG1();
		DISASS("%08x   %08x  mfspr\t r%d, SPRG2\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_SPRG2:
		val_RT = cpus->getSPRG2();
		DISASS("%08x   %08x  mfspr\t r%d, SPRG2\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_SDR1:
		val_RT = cpus->getSDR1();
		DISASS("%08x   %08x  mfspr\t r%d, SDR1\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_SRR0:
		val_RT = cpus->getSRR0();
		DISASS("%08x   %08x  mfspr\t r%d, SRR0\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_SRR1:
		val_RT = cpus->getSRR1();
		DISASS("%08x   %08x  mfspr\t r%d, SRR1\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_DSISR:
		val_RT = cpus->getDSISR();
		DISASS("%08x   %08x  mfspr\t r%d, DSISR\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_DAR:
		val_RT = cpus->getDAR();
		DISASS("%08x   %08x  mfspr\t r%d, DAR\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_DEC:
		val_RT = cpus->getDEC();
		DISASS("%08x   %08x  mfspr\t r%d, DEC\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	case SPR_DABR:
		val_RT = 0;
		DISASS("%08x   %08x  mfspr\t r%d, DABR\t[%08x] *** DUMMY ***\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

		/* IMPDEF SRs */
	case SPR_HID0:
		val_RT = cpus->getHID0();
		DISASS("%08x   %08x  mfspr\t r%d, HID0\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_HID1:
		val_RT = cpus->getHID1();
		DISASS("%08x   %08x  mfspr\t r%d, HID1\t[%08x]\n", pc, inst, RT, val_RT);
		CHECK_YOUR_PRIVILEGE();
		break;

	default:
		FATAL("%08x   %08x  mfspr(%d) unimplemented\n", pc, inst, spr);
		break;
	}

	WRITE_GPR(RT, val_RT);

	incPC();
	COUNT(CTR_INST_MFSPR);
	INTERP_RETURN;
}

void PPCInterpreter::helper_DEBUG(REG val)
{
        switch(val & 0xff00) {
		case SPR_DEBUG_EXIT: {
			/* Dump machine state in a format useful for random test generation:
			 */
			for (int i = 0; i < 32; i++) {
				printf("GPR%d %016llx\n", i, (uint64_t)cpus->getGPR(i));
			}
			printf("CR %016llx\n", (uint64_t)cpus->getCR());
			printf("LR %016llx\n", (uint64_t)cpus->getLR());
			printf("CTR %016llx\n", (uint64_t)cpus->getCTR());
			printf("XER %016llx\n", (uint64_t)cpus->getXER());
			SIM_QUIT(val & 0xff);
                } break;

		case SPR_DEBUG_PUTC:
			putchar(val & 0xff);
                break;

		default:
			FATAL("Unknown DEBUG val %08x\n", val);
        }
}

RET_TYPE PPCInterpreter::op_mftb(int spr, int RT)
{
	REG val_RT = 0;

	/* RT=TB{U}[spr] */
	switch (spr) {
	case SPR_TBU:
		val_RT = cpus->getTB() >> 32;
		DISASS("%08x   %08x  mftbu\t r%d, TBU\t[%08x]\n", pc, inst, RT, val_RT);
		break;
	case SPR_TB:
		val_RT = cpus->getTB() & 0xffffffff;
		DISASS("%08x   %08x  mftbl\t r%d, TB\t[%08x]\n", pc, inst, RT, val_RT);
		break;

	default:
		FATAL("%08x   %08x  mftb(%d) unimplemented\n", pc, inst, spr);
		break;
	}

	WRITE_GPR(RT, val_RT);

	incPC();
	COUNT(CTR_INST_MFTB);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mtspr(int spr, int RS)
{
	// Inputs 'spr,RS', implicit inputs ''
	REG val_RS = READ_GPR(RS);

	/* SPRS[spr] = RS */

	switch (spr) {
		/* UISA */
	case SPR_LR:
		cpus->setLR(val_RS);
		DISASS("%08x   %08x  mtlr\t r%d\t[%08x]\n", pc, inst, RS, val_RS);
		break;
	case SPR_CTR:
		cpus->setCTR(val_RS);
		DISASS("%08x   %08x  mtctr\t r%d\t[%08x]\n", pc, inst, RS, val_RS);
		break;
	case SPR_XER:
		cpus->setXER(val_RS);
		DISASS("%08x   %08x  mtxer\t r%d\t[%08x]\n", pc, inst, RS, val_RS);
		break;

		/* VEA, privileged */
	case SPR_TB_W:
		DISASS("%08x   %08x  mtspr\t TB, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setTB(val_RS | (cpus->getTB() & 0xffffffff00000000));
		break;
	case SPR_TBU_W:
		DISASS("%08x   %08x  mtspr\t TBU, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setTB((((u64)val_RS) << 32) | (cpus->getTB() & 0xffffffff));
		break;

		/* OEA, privileged */
		/* BAT registers, I and D, 0 to 7. */
	case SPR_IBAT0L:
		DISASS("%08x   %08x  mtibatl\t 0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(0, val_RS);
		break;
	case SPR_IBAT0U:
		DISASS("%08x   %08x  mtibatu\t 0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(0, val_RS);
		break;
	case SPR_IBAT1L:
		DISASS("%08x   %08x  mtibatl\t 1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(1, val_RS);
		break;
	case SPR_IBAT1U:
		DISASS("%08x   %08x  mtibatu\t 1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(1, val_RS);
		break;
	case SPR_IBAT2L:
		DISASS("%08x   %08x  mtibatl\t 2, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(2, val_RS);
		break;
	case SPR_IBAT2U:
		DISASS("%08x   %08x  mtibatu\t 2, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(2, val_RS);
		break;
	case SPR_IBAT3L:
		DISASS("%08x   %08x  mtibatl\t 3, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(3, val_RS);
		break;
	case SPR_IBAT3U:
		DISASS("%08x   %08x  mtibatu\t 3, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(3, val_RS);
		break;
	case SPR_IBAT4L:
		DISASS("%08x   %08x  mtibatl\t 4, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(4, val_RS);
		break;
	case SPR_IBAT4U:
		DISASS("%08x   %08x  mtibatu\t 4, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(4, val_RS);
		break;
	case SPR_IBAT5L:
		DISASS("%08x   %08x  mtibatl\t 5, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(5, val_RS);
		break;
	case SPR_IBAT5U:
		DISASS("%08x   %08x  mtibatu\t 5, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(5, val_RS);
		break;
	case SPR_IBAT6L:
		DISASS("%08x   %08x  mtibatl\t 6, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(6, val_RS);
		break;
	case SPR_IBAT6U:
		DISASS("%08x   %08x  mtibatu\t 6, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(6, val_RS);
		break;
	case SPR_IBAT7L:
		DISASS("%08x   %08x  mtibatl\t 7, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATlower(7, val_RS);
		break;
	case SPR_IBAT7U:
		DISASS("%08x   %08x  mtibatu\t 7, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setIBATupper(7, val_RS);
		break;

	case SPR_DBAT0L:
		DISASS("%08x   %08x  mtdbatl\t 0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(0, val_RS);
		break;
	case SPR_DBAT0U:
		DISASS("%08x   %08x  mtdbatu\t 0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(0, val_RS);
		break;
	case SPR_DBAT1L:
		DISASS("%08x   %08x  mtdbatl\t 1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(1, val_RS);
		break;
	case SPR_DBAT1U:
		DISASS("%08x   %08x  mtdbatu\t 1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(1, val_RS);
		break;
	case SPR_DBAT2L:
		DISASS("%08x   %08x  mtdbatl\t 2, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(2, val_RS);
		break;
	case SPR_DBAT2U:
		DISASS("%08x   %08x  mtdbatu\t 2, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(2, val_RS);
		break;
	case SPR_DBAT3L:
		DISASS("%08x   %08x  mtdbatl\t 3, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(3, val_RS);
		break;
	case SPR_DBAT3U:
		DISASS("%08x   %08x  mtdbatu\t 3, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(3, val_RS);
		break;
	case SPR_DBAT4L:
		DISASS("%08x   %08x  mtdbatl\t 4, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(4, val_RS);
		break;
	case SPR_DBAT4U:
		DISASS("%08x   %08x  mtdbatu\t 4, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(4, val_RS);
		break;
	case SPR_DBAT5L:
		DISASS("%08x   %08x  mtdbatl\t 5, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(5, val_RS);
		break;
	case SPR_DBAT5U:
		DISASS("%08x   %08x  mtdbatu\t 5, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(5, val_RS);
		break;
	case SPR_DBAT6L:
		DISASS("%08x   %08x  mtdbatl\t 6, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(6, val_RS);
		break;
	case SPR_DBAT6U:
		DISASS("%08x   %08x  mtdbatu\t 6, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(6, val_RS);
		break;
	case SPR_DBAT7L:
		DISASS("%08x   %08x  mtdbatl\t 7, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATlower(7, val_RS);
		break;
	case SPR_DBAT7U:
		DISASS("%08x   %08x  mtdbatu\t 7, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		mmu->setDBATupper(7, val_RS);
		break;

	case SPR_SPRG0:
		DISASS("%08x   %08x  mtspr\t SPRG0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSPRG0(val_RS);
		break;
	case SPR_SPRG1:
		DISASS("%08x   %08x  mtspr\t SPRG1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSPRG1(val_RS);
		break;
	case SPR_SPRG2:
		DISASS("%08x   %08x  mtspr\t SPRG2, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSPRG2(val_RS);
		break;
	case SPR_SPRG3:
		DISASS("%08x   %08x  mtspr\t SPRG3, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSPRG3(val_RS);
		break;

	case SPR_SRR0:
		DISASS("%08x   %08x  mtspr\t SRR0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSRR0(val_RS);
		break;
	case SPR_SRR1:
		DISASS("%08x   %08x  mtspr\t SRR1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSRR1(val_RS);
		break;

	case SPR_DSISR:
		DISASS("%08x   %08x  mtspr\t DSISR, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setDSISR(val_RS);
		break;
	case SPR_DAR:
		DISASS("%08x   %08x  mtspr\t DAR, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setDAR(val_RS);
		break;

	case SPR_SDR1:
		DISASS("%08x   %08x  mtspr\t SDR1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setSDR1(val_RS);
		mmu->setSDR1(val_RS);
		break;

	case SPR_DEC:
		DISASS("%08x   %08x  mtspr\t DEC, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		cpus->setDEC(val_RS);
		break;

	case SPR_DABR:
		DISASS("%08x   %08x  mtspr\t DABR, r%d\t[%08x] *** IGNORED ***\n",
		       pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		break;

		/* IMPDEF SRs */
	case SPR_HID0:
		DISASS("%08x   %08x  mtspr\t HID0, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE(); /* This should be HYP (once supported) */
		cpus->setHID0(val_RS);
		break;
	case SPR_HID1:
		DISASS("%08x   %08x  mtspr\t HID1, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE(); /* This should be HYP (once supported) */
		cpus->setHID1(val_RS);
		break;

        case SPR_DEBUG:
		DISASS("%08x   %08x  mtspr\t DEBUG, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE(); /* This should be HYP (once supported) */
		helper_DEBUG(val_RS);
                break;
	case SPR_DC_INV_SET:
		DISASS("%08x   %08x  mtspr\t DC_INV_SET, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		break;
	case SPR_IC_INV_SET:
		DISASS("%08x   %08x  mtspr\t DC_INV_SET, r%d\t[%08x]\n", pc, inst, RS, val_RS);
		CHECK_YOUR_PRIVILEGE();
		/* Interpreter can ignore this, but JIT needs to heed this! */
#if ENABLE_JIT != 0
		FATAL("Need to implement I$ invalidate for JIT\n");
#endif
		break;

	default:
		FATAL("%08x   %08x  mtspr(%d) unimplemented\n", pc, inst, spr);
		break;
	}

	incPC();
	COUNT(CTR_INST_MTSPR);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_rfi()
{
	VA	new_pc = cpus->getSRR0() & ~3;
	u32	msr = cpus->getSRR1();
	DISASS("%08x   %08x  rfi\t MSR=%08x, PC=%08x  \n", pc, inst, msr, new_pc);

	CHECK_YOUR_PRIVILEGE();

	blow_reservation();

	/* Return from exception:
	 * Actually pretty simple - just move SRR1 to MSR and SRR0 to PC!
	 */
	cpus->setPC(new_pc);
	WRITE_MSR(msr);

	COUNT(CTR_INST_RFI);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_sync(int L, int E)
{
	/* Barrier:  For the purposes of this ISS, this does nothing. */

	if (E != 0) {
		DISASS("%08x   %08x  mb(%x)\n", pc, inst, E);	// "Elemental barriers"
	} else if (L == 0) {
		DISASS("%08x   %08x  sync\n", pc, inst);
	} else if (L == 1) {
		DISASS("%08x   %08x  lwsync\n", pc, inst);
	} else if (L == 2) {
		DISASS("%08x   %08x  ptesync\n", pc, inst);
	} else {
		DISASS("%08x   %08x  *UNKNOWN* sync\t %d, %d \n", pc, inst, /* in */ L, /* in */ E);
	}

	incPC();
	COUNT(CTR_INST_SYNC);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mfsr(int SR, int RT)
{
	// Inputs 'SR', implicit inputs ''
	REG val_RT = 0;

	DISASS("%08x   %08x  mfsr\t %d,  %d \n", pc, inst, /* out */ RT, /* in */ SR);

	CHECK_YOUR_PRIVILEGE();

	val_RT = mmu->getSegmentReg(SR);

	// Outputs 'RT', implicit outputs ''
	WRITE_GPR(RT, val_RT);

	incPC();
	COUNT(CTR_INST_MFSR);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mfsrin(int RB, int RT)
{
	// Inputs 'RB', implicit inputs ''
	REG val_RB = READ_GPR(RB);
	REG val_RT = 0;

	DISASS("%08x   %08x  mfsrin\t %d,  %d \n", pc, inst, /* out */ RT, /* in */ RB);

	CHECK_YOUR_PRIVILEGE();

	val_RT = mmu->getSegmentReg((val_RB >> 28) & 0xf);

	// Outputs 'RT', implicit outputs ''
	WRITE_GPR(RT, val_RT);

	incPC();
	COUNT(CTR_INST_MFSRIN);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mtsr(int SR, int RS)
{
	// Inputs 'SR,RS', implicit inputs ''
	REG val_RS = READ_GPR(RS);

	DISASS("%08x   %08x  mtsr\t %d, %d \n", pc, inst, /* in */ SR, /* in */ RS);

	CHECK_YOUR_PRIVILEGE();

	mmu->setSegmentReg(SR, val_RS);

	incPC();
	COUNT(CTR_INST_MTSR);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_mtsrin(int RS, int RB)
{
	// Inputs 'RS,RB', implicit inputs ''
	REG val_RS = READ_GPR(RS);
	REG val_RB = READ_GPR(RB);

	DISASS("%08x   %08x  mtsrin\t %d, %d \n", pc, inst, /* in */ RS, /* in */ RB);

	CHECK_YOUR_PRIVILEGE();

	mmu->setSegmentReg((val_RB >> 28) & 0xf, val_RS);

	incPC();
	COUNT(CTR_INST_MTSRIN);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_tlbie(int RB, int L)
{
	DISASS("%08x   %08x  tlbie	r%d, %d\n", pc, inst, RB, L);

	/* TODO: Make this better. */
	mmu->tlbie(); /* No args yet, doesn't really do fine-grained inval! */

	incPC();
	COUNT(CTR_INST_TLBIE);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_tlbiel(int RB, int L)
{
	DISASS("%08x   %08x  tlbiel	r%d, %d\n", pc, inst, RB, L);

	/* TODO: Make this better.  I don't think this is a 604 instr. */
	mmu->tlbie(); /* No args yet, doesn't really do fine-grained inval! */

	incPC();
	COUNT(CTR_INST_TLBIEL);
	INTERP_RETURN;
}

/* Simple debug output ...
 * Matches opcode 0x1400008a with RA
 * r0 = 0;  exit
 * r0 = 1;  putch(r3)
 */
RET_TYPE PPCInterpreter::op_DEBUG(int RA)
{
	// Inputs 'RA', implicit inputs r0, r3, etc.
	REG val_RA = READ_GPR(RA);
	REG op = READ_GPR(0);

	DISASS("%08x   %08x  DEBUG\t %d \n", pc, inst, /* in */ RA);

	static bool first = true;

	if (first) {
		// cpus->dump();
		// stats_dump();
		first = false;
	}

	switch(op) {
		case 0:
			/* This exits sim. */
			SIM_QUIT(val_RA);
			break;

		case 1: {
			REG c = READ_GPR(3);
			putchar(c);
		} break;

		case 2: // trace on
			printf("Tracing on\n");
			CFG(dump_state_period) = 1;
			log_enables_mask |= LOG_FLAG_DISASS;
			break;

		default:
			break;
	}

	incPC();
	COUNT(CTR_INST_EXIT);
	INTERP_RETURN;
}

RET_TYPE PPCInterpreter::op_icbi(int RA0, int RB)
{
	REG val_RA0 = READ_GPR_OZ(RA0);
	REG val_RB = READ_GPR(RB);

	DISASS("%08x   %08x  icbi\t r%d, r%d\n", pc, inst, /* in */ RA0, /* in */ RB);

	/* FIXME: Inform blockstore and get it to clear itself, translating VA
	 * to PA first.
	 *
	 * This will probably need to stash that and then action that after the
	 * block completes.  (Don't want to reset blockstore whilst executing
	 * out of it!)
	 */
	u64 pa;
	VA va = val_RA0+val_RB;
	PPCMMU::fault_t fault;
	if (!mmu->translateAddr(va, &pa, false, true, cpus->isPrivileged(), &fault)) {
                cpus->raiseMemException(true, false, va, fault, 0);
		INTERP_RETURN;
	}
	/* FIXME: blockstore_invalidate_range(pa, pa+32); */

	incPC();
	COUNT(CTR_INST_ICBI);
	INTERP_RETURN;
}
