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

#ifndef ARCH_DEFNS_H
#define ARCH_DEFNS_H

// Implementation details:

#define MR_PVR		0xbb0a0001	/* MattRISC, similar to 604 */
// 0x00090204 /* 604e */
// 0x00080100 /* PPC740 (G3) */
// 603 doesn't do SDR1/HTAB access; 604 does, G3 does.

// Arch definitions

// XER contents
#define XER_SO		0x80000000
#define XER_OV		0x40000000
#define XER_CA		0x20000000
#define XER_STR		0x0000007f

// User-accessible SPRs
#define SPR_CTR		9
#define SPR_LR		8
#define SPR_PVR		287
#define SPR_TB		268 	// RO
#define SPR_TBU		269	// RO
#define SPR_XER		1
// Supervisor SPRs - writing TB
#define SPR_TB_W	284
#define SPR_TBU_W	285

#define SPR_DBAT0U	536
#define SPR_DBAT0L	537
#define SPR_DBAT1U	538
#define SPR_DBAT1L	539
#define SPR_DBAT2U	540
#define SPR_DBAT2L	541
#define SPR_DBAT3U	542
#define SPR_DBAT3L	543
#define SPR_DBAT4U	568
#define SPR_DBAT4L	569
#define SPR_DBAT5U	570
#define SPR_DBAT5L	571
#define SPR_DBAT6U	572
#define SPR_DBAT6L	573
#define SPR_DBAT7U	574
#define SPR_DBAT7L	575

#define SPR_IBAT0U	528
#define SPR_IBAT0L	529
#define SPR_IBAT1U	530
#define SPR_IBAT1L	531
#define SPR_IBAT2U	532
#define SPR_IBAT2L	533
#define SPR_IBAT3U	534
#define SPR_IBAT3L	535
#define SPR_IBAT4U	560
#define SPR_IBAT4L	561
#define SPR_IBAT5U	562
#define SPR_IBAT5L	563
#define SPR_IBAT6U	564
#define SPR_IBAT6L	565
#define SPR_IBAT7U	566
#define SPR_IBAT7L	567

#define SPR_DAR		19
#define SPR_DEC		22
#define SPR_DSISR	18
#define SPR_SDR1	25
#define SPR_SRR0	26
#define SPR_SRR1	27

#define SPR_SPRG0	272
#define SPR_SPRG1	273
#define SPR_SPRG2	274
#define SPR_SPRG3	275

// IMPDEF SPRs
// These are 604e-specific:
#define SPR_HID0	1008
#define SPR_HID1	1009
#define SPR_MMCR0	952
#define SPR_MMCR1	956
#define SPR_PMC1	953
#define SPR_PMC2	954
#define SPR_PMC3	957
#define SPR_PMC4	958
#define SPR_SDA		959
#define SPR_SIA		955

// My debug stuff
#define SPR_DEBUG       1023
#define SPR_DEBUG_EXIT  0x0000
#define SPR_DEBUG_PUTC  0x0100

// MR IMPDEF SRs
#define SPR_DC_INV_SET  1022
#define SPR_IC_INV_SET  1021

// PPC optional SPRs
#define SPR_EAR		282
#define SPR_PIR		1023
#define SPR_DABR	1013

// MSR
#define MSR_LE		B(63-63)
#define MSR_RI		B(63-62)
#define MSR_DR		B(63-59)
#define MSR_IR		B(63-58)
#define MSR_IP		B(63-57)
#define MSR_FP		B(63-50)
#define MSR_PR		B(63-49)
#define MSR_EE		B(63-48)

// CPU-specific.  These should move away, in time.
typedef u32		VA;
typedef u32		PA;
#define FMT_PA		"%08x"

// Bitness
typedef u32		REG;
typedef s32		sREG;
#define FMT_REG		"%08x"

#endif
