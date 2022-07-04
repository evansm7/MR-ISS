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

#ifndef PPCMMU_H
#define PPCMMU_H

#include "types.h"
#include "Bus.h"
#include "stats.h"
#include "utility.h"

#include <string.h>

#define PPCMMU_EA_SIZE		32
#define PPCMMU_SEGMENT_SIZE	MiB(256)
#define PPCMMU_NR_BATS		4
#define PPCMMU_NR_SEGS		16

#define PPC_PTE_V		0x80000000
#define PPC_PTE_H		0x00000040

#define PPCMMU_HVA_IO_BIT	B(63)

class PPCMMU {
public:
        PPCMMU();

	////////////////////////////////////////////////////////////////////////////////
	// Types

	typedef enum {
		FAULT_NONE = 0,
		FAULT_NO_PAGE,
		FAULT_NO_SEGMENT,
		FAULT_PERMS,
		FAULT_PERMS_NX, /* Specifically for i-fetch to seg w/ n=1 */
                FAULT_ALIGN,
	} fault_t;

	////////////////////////////////////////////////////////////////////////////////
	// Standard memory accessors; return true on fault.
	fault_t	loadInst32(VA addr, u32 *dest, bool priv);
	fault_t	load32(VA addr, u32 *dest, bool priv);
	fault_t	load16(VA addr, u16 *dest, bool priv);
	fault_t	load8(VA addr, u8 *dest, bool priv);
	fault_t	store32(VA addr, u32 val, bool priv);
	fault_t	store16(VA addr, u16 val, bool priv);
	fault_t	store8(VA addr, u8 val, bool priv);

	////////////////////////////////////////////////////////////////////////////////
	// Core translation
	// Returns true on success, else false and returns fault.
	inline bool translateAddr(VA addr, u64 *pa, bool InD, bool RnW, bool priv, fault_t *fault_out)
	{
#if DUMMY_MEM_ACCESS == 0
		mmuperms_t perms;
		fault_t fault;
		/* Check utlb first; if miss, translate (insert).
                 *
                 * Translations are segregated by I/D and by
                 * privilege.  For example, for an instruction fetch
                 * to a given EA, an IBAT might match (leading to a
                 * utlb entry based solely on that info) whereas a
                 * data access to the same address might not match
                 * DBAT, and create a utlb entry based on a HTAB
                 * lookup.  Effectively, this means separate utlbs for
                 * I and D.
                 *
                 * Similarly, a privileged access might match a BAT
                 * whereas an unprivileged access to the same EA might
                 * not (Vs/Vp bits differ).
                 *
                 * This scheme misses sharing opportunities (e.g. pages
                 * that are kernel RW/user RO need 2x walks) but
                 * simple(r) & correct wins.
                 *
                 * Permissions become simple R+W per utlb entry; execute
                 * is an Iutlb entry having R=1.
                 */
		if (unlikely(!utlbLookup(InD, priv, addr, pa, &perms))) {
			PA scratch_pa;
                do_lookup:
			if (!translateEA(addr, InD, RnW, priv, &scratch_pa, &perms, &fault)) {
				MMUTRACE("Fault %d on %s %s from %p\n", fault,
					 (InD) ? "inst" : "data",
					 (RnW) ? "read" : "write",
					 addr);
				*fault_out = fault;
				return false;
			}
			/* translateEA inserted a TLB entry; get it. */
			if (!utlbLookup(InD, priv, addr, pa, &perms)) {
				FATAL("TLB miss after insert, addr %lx\n", addr);
			}
		}

		if (unlikely(!checkPerms(perms, RnW, InD))) {
                        /* One reason checkPerms can fail is C=0, making a write fail.
                         * The TLB entry might have been successfully used for many reads before this,
                         * but now needs to be converted to a writeable form (by re-walking
                         * _for write_ and setting C).
                         */
                        if (checkFaultWasCleanliness(perms, RnW, InD)) {
                                MMUTRACE("Permission fault on %p was C=0, re-walking\n", addr);
                                goto do_lookup;
                        } else {
                                // Genuine permission fault
                                *fault_out = FAULT_PERMS;
                                MMUTRACE("Permission fault on %sprivileged %s %s from %p "
                                         "(r%d w%d clean%d)\n",
                                         priv ? "" : "un", (InD) ? "inst" : "data", (RnW) ? "read" : "write",
                                         addr, perms.r, perms.w, perms.clean);
                                return false;
                        }
		}
#else
                *pa = addr;
#endif
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Mappings, maintenance, etc.
	// invalidate
	// Endianness

	void	setSDR1(REG val);

	void	setIBATupper(unsigned int bat, u32 val);
	void	setIBATlower(unsigned int bat, u32 val);
	void	setDBATupper(unsigned int bat, u32 val);
	void	setDBATlower(unsigned int bat, u32 val);

	void	setSegmentReg(unsigned int sr, u32 val);

	u32	getIBATupper(unsigned int bat);
	u32	getIBATlower(unsigned int bat);
	u32	getDBATupper(unsigned int bat);
	u32	getDBATlower(unsigned int bat);

	u32	getSegmentReg(unsigned int sr);

	void	tlbia();
	void	tlbie();

	void	setIRDR(bool ir, bool dr);

	unsigned int	getGenCount()	{ return generation_count; }

	////////////////////////////////////////////////////////////////////////////////
	// Backend:  physical memory access via bus
	void	setBus(Bus *b)		{ bus = b; }

private:
	Bus	*bus;
	bool	enabled_i;
	bool	enabled_d;

	bool	isBigEndian()		{ return true; }

	PA	htab_phys;
	u32	htab_mask;

	////////////////////////////////////////////////////////////////////////////////
	// Internal types:
	typedef struct {
		u32	bepi;	/* Stored unshifted, i.e. in [31:17] */
		u32	bl_shift;
		bool	vs;
		bool	vp;
		u32	brpn;	/* Stored unshifted */
		u32	wimg;
		u32	pp;
	} bat_t;

	typedef struct {
		u32	vsid;
		bool	ks;
		bool	kp;
		bool	n;
	} seg_t;

	typedef struct {
		union {
			struct {
				unsigned int r : 1;
				unsigned int w : 1;
                                unsigned int clean : 1;
			};
			u32 field;
		};
	} mmuperms_t;
#define PPCMMU_PERMS_ANY(x)     do { (x)->field = 0; (x)->r = 1; (x)->w = 1; (x)->clean = 0; \
        } while(0)


#define PPCMMU_UTLB_TR		1
#define PPCMMU_UTLB_VALID 	2
	class utlb_t {
	public:
		utlb_t() { }

		/* ea bit [0] = IR/DR on, [1] = valid */
		u32 ea;
		/* pa bits [7:0] = perms */
		u64 pa;

		bool isValid()	{ return ea & PPCMMU_UTLB_VALID; }
		u8 getPerms() 	{ return pa & 0x0ff; }
		u64 getPA()	{ return pa & ~0xfff; }
		PA getEA()	{ return ea; }
		void set(VA _ea, int r, u64 _pa, u8 _perms)
		{
			ea = (_ea & ~0xfff) | r | PPCMMU_UTLB_VALID;
			pa = (_pa & ~0xfff) | _perms;
		}
	};

	/* Include actual utlb implementation: */
#include "PPCMMU_utlb_dm.h"
	/* That defines the following:
	 * void		iutlbInv();
	 * void		dutlbInv();
	 * void		dumpUTLBs();
	 * bool    	utlbLookup(bool InD, bool priv, VA addr, PA *output_addr, mmuperms_t *perms);
	 * void    	utlbInsert(bool InD, bool priv, VA addr, PA out_addr, u32 perms);
	 */

	////////////////////////////////////////////////////////////////////////////////
	// Internal data:
	bat_t	ibat[PPCMMU_NR_BATS];
	bat_t	dbat[PPCMMU_NR_BATS];
	seg_t	segreg[PPCMMU_NR_SEGS];

	unsigned int generation_count;

	////////////////////////////////////////////////////////////////////////////////

	bool	translateEA(VA addr, bool InD, bool RnW, bool priv, PA *output_addr, mmuperms_t *perms, fault_t *fault_type);
	bool	matchBAT(VA addr, bool InD, bool priv, PA *output_addr, mmuperms_t *perms);

	/* Are the given perms accessible for RWX given the privilege?
	 * Returns true if so, false if permission fault.
	 */
	bool	checkPerms(mmuperms_t perms, bool RnW, bool InD)
	{
                if (InD || RnW)
                        return !!perms.r;
                else // !InD && !RnW, i.e. store -- ignores InD
                        return !!perms.w
#ifdef UPDATE_RC
                                && !perms.clean
#endif
                                ;
	}
        /* Return true if a perms fault was only due to C being 0: */
        bool    checkFaultWasCleanliness(mmuperms_t perms, bool RnW, bool InD)
        {
#ifdef UPDATE_RC
                return (!RnW && !InD) && (!!perms.w) && perms.clean;
#else
                return false;
#endif
        }
};

#endif
