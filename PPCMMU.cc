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

#include "PPCMMU.h"
#include "log.h"
#include "utility.h"
#include "inst_utility.h"

#include <arpa/inet.h>
#include <string.h>

#define BS32(x)  ( isBigEndian() ? htonl(x) : (x) )
#define BS16(x)  ( isBigEndian() ? htons(x) : (x) )

#define IS_DIRECT(x)	!((x) & PPCMMU_HVA_IO_BIT)

PPCMMU::PPCMMU() :
	         enabled_i(false)
		,enabled_d(false)
{
	/* Zero le BATs */
	int i;
	for (i = 0; i < PPCMMU_NR_BATS; i++) {
		ibat[i].vs = 0;
		ibat[i].vp = 0;
		dbat[i].vs = 0;
		dbat[i].vp = 0;
	}
	iutlbInv();
	dutlbInv();

	generation_count = 0;
}

PPCMMU::fault_t	PPCMMU::loadInst32(VA addr, u32 *dest, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	*dest = BS32(bus->read32(pa));
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, true, true, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*dest = BS32(*(u32 *)pa);
	} else {
		*dest = BS32(bus->read32(pa));
	}
#endif
	COUNT(CTR_MEM_RI);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::load32(VA addr, u32 *dest, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	*dest = BS32(bus->read32(pa));
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, true, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*dest = BS32(*(u32 *)pa);
	} else {
		*dest = BS32(bus->read32(pa));
	}
#endif
	if (pa & 3) {
		COUNT(CTR_MEM_R32_UNALIGNED);
		if ((pa & 7) > 4) {
			// Count crossing 8-byte boundary
			COUNT(CTR_MEM_R32_UNALIGNED_CROSS8);
#if MEM_OP_ALIGNMENT == 1
                        return FAULT_ALIGN;
#endif
                }
#if MEM_OP_ALIGNMENT == 0
                return FAULT_ALIGN;
#endif
	}
	COUNT(CTR_MEM_R32);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::load16(VA addr, u16 *dest, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	*dest = BS16(bus->read16(pa));
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, true, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*dest = BS16(*(u16 *)pa);
	} else {
		*dest = BS16(bus->read16(pa));
	}
#endif
	if (pa & 1) {
		COUNT(CTR_MEM_R16_UNALIGNED);
		if ((pa & 7) == 7) {
			COUNT(CTR_MEM_R16_UNALIGNED_CROSS8);
#if MEM_OP_ALIGNMENT == 1
                        return FAULT_ALIGN;
#endif
                }
#if MEM_OP_ALIGNMENT == 0
                return FAULT_ALIGN;
#endif
	}
	COUNT(CTR_MEM_R16);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::load8(VA addr, u8 *dest, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	*dest = bus->read8(pa);
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, true, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*dest = *(u8 *)pa;
	} else {
		*dest = bus->read8(pa);
	}
#endif
	COUNT(CTR_MEM_R8);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::store32(VA addr, u32 val, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	bus->write32(pa, BS32(val));
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, false, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*(u32 *)pa = BS32(val);
	} else {
		bus->write32(pa, BS32(val));
	}
#endif
	if (pa & 3) {
		COUNT(CTR_MEM_W32_UNALIGNED);
		if ((pa & 7) > 4) {
			COUNT(CTR_MEM_W32_UNALIGNED_CROSS8);
#if MEM_OP_ALIGNMENT == 1
                        return FAULT_ALIGN;
#endif
                }
#if MEM_OP_ALIGNMENT == 0
                return FAULT_ALIGN;
#endif
	}
	COUNT(CTR_MEM_W32);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::store16(VA addr, u16 val, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	bus->write16(pa, BS16(val));
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, false, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*(u16 *)pa = BS16(val);
	} else {
		bus->write16(pa, BS16(val));
	}
#endif
	if (pa & 1) {
		COUNT(CTR_MEM_W16_UNALIGNED);
		if ((pa & 7) == 7) {
			COUNT(CTR_MEM_W16_UNALIGNED_CROSS8);
#if MEM_OP_ALIGNMENT == 1
                        return FAULT_ALIGN;
#endif
                }
#if MEM_OP_ALIGNMENT == 0
                return FAULT_ALIGN;
#endif
	}
	COUNT(CTR_MEM_W16);
	return FAULT_NONE;
}

PPCMMU::fault_t	PPCMMU::store8(VA addr, u8 val, bool priv)
{
#if FLAT_MEM
	PA pa = addr;
	bus->write8(pa, val);
#else
	u64 pa;
	fault_t f;
	if (!translateAddr(addr, &pa, false, false, priv, &f)) {
		return f;
	}
	if (likely(IS_DIRECT(pa))) {
		*(u8 *)pa = val;
	} else {
		bus->write8(pa, val);
	}
#endif
	COUNT(CTR_MEM_W8);
	return FAULT_NONE;
}

////////////////////////////////////////////////////////////////////////////////

void	PPCMMU::setSDR1(REG val)
{
	// TODO: Blow away ERAT-like things...
	htab_phys = val & 0xffff0000;
	htab_mask = val & 0x1ff;
	MMUTRACE("SDR1 = %08x\t htab_phys = %08x, htab_mask = %08x\n",
		 val, htab_phys, htab_mask);
	iutlbInv();
	dutlbInv();
	generation_count++;
}

static	unsigned int m_fls(u32 x)
{
	return x ? 32-__builtin_clz(x) : 0;
}

void	PPCMMU::setIBATupper(unsigned int bat, u32 val)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	ibat[bat].bepi = val & 0xfffe0000;
	ibat[bat].bl_shift = m_fls((val >> 2) & 0x7ff);
	ibat[bat].vs = val & 2;
	ibat[bat].vp = val & 1;
	MMUTRACE("IBAT_U[%d] = %08x\tbepi %08x, bl_sh %d, vs %d, vp %d\n",
		 bat, val, ibat[bat].bepi, ibat[bat].bl_shift,
		 ibat[bat].vs, ibat[bat].vp);
	iutlbInv();
	generation_count++;
}

void	PPCMMU::setIBATlower(unsigned int bat, u32 val)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	ibat[bat].brpn = val & 0xfffe0000;
	ibat[bat].wimg = val & 0x78;
	ibat[bat].pp = val & 3;
	MMUTRACE("IBAT_L[%d] = %08x\tbrpn %08x, wimg 0x%x, pp %d\n",
		 bat, val, ibat[bat].brpn, ibat[bat].wimg, ibat[bat].pp);
	iutlbInv();
	generation_count++;
}

void	PPCMMU::setDBATupper(unsigned int bat, u32 val)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	dbat[bat].bepi = val & 0xfffe0000;
	dbat[bat].bl_shift = m_fls((val >> 2) & 0x7ff);
	dbat[bat].vs = val & 2;
	dbat[bat].vp = val & 1;
	MMUTRACE("DBAT_U[%d] = %08x\tbepi %08x, bl_sh %d, vs %d, vp %d\n",
		 bat, val, dbat[bat].bepi, dbat[bat].bl_shift,
		 dbat[bat].vs, dbat[bat].vp);
	dutlbInv();
	generation_count++;
}

void	PPCMMU::setDBATlower(unsigned int bat, u32 val)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	dbat[bat].brpn = val & 0xfffe0000;
	dbat[bat].wimg = val & 0x78;
	dbat[bat].pp = val & 3;
	MMUTRACE("DBAT_L[%d] = %08x\tbrpn %08x, wimg 0x%x, pp %d\n",
		 bat, val, dbat[bat].brpn, dbat[bat].wimg, dbat[bat].pp);
	dutlbInv();
	generation_count++;
}

u32	PPCMMU::getIBATupper(unsigned int bat)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	u32 r = ibat[bat].bepi;
	r |= ((1 << ibat[bat].bl_shift) - 1) << 2;
	r |= ibat[bat].vs ? 2 : 0;
	r |= ibat[bat].vp ? 1 : 0;
	return r;
}

u32	PPCMMU::getIBATlower(unsigned int bat)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	return ibat[bat].brpn | ibat[bat].wimg | ibat[bat].pp;
}

u32	PPCMMU::getDBATupper(unsigned int bat)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	u32 r = dbat[bat].bepi;
	r |= ((1 << dbat[bat].bl_shift) - 1) << 2;
	r |= dbat[bat].vs ? 2 : 0;
	r |= dbat[bat].vp ? 1 : 0;
	return r;
}

u32	PPCMMU::getDBATlower(unsigned int bat)
{
	ASSERT(bat < PPCMMU_NR_BATS);
	return dbat[bat].brpn | dbat[bat].wimg | dbat[bat].pp;
}

void	PPCMMU::setSegmentReg(unsigned int sr, u32 val)
{
	ASSERT(sr < PPCMMU_NR_SEGS);

	segreg[sr].vsid = val & 0x00ffffff;
	segreg[sr].ks = !!(val & B(30));
	segreg[sr].kp = !!(val & B(29));
	segreg[sr].n = !!(val & B(28));

	MMUTRACE("SEGREG[%d] = %08x\t vsid = 0x%08x, ks %d, kp %d, n %d\n",
		 sr, val, segreg[sr].vsid,
		 segreg[sr].ks, segreg[sr].kp, segreg[sr].n);

	dutlbInv();
	iutlbInv();
	generation_count++;
}

u32	PPCMMU::getSegmentReg(unsigned int sr)
{
	ASSERT(sr < PPCMMU_NR_SEGS);
	seg_t s = segreg[sr];
	u32 r = s.vsid;
	if (s.ks)
		r |= B(30);
	if (s.kp)
		r |= B(29);
	if (s.n)
		r |= B(28);
	return r;
}

/* TLB invalidation */
void	PPCMMU::tlbia()
{
	MMUTRACE("TLBIA\n");
	dutlbInv();
	iutlbInv();
	generation_count++;
}

/* TLB entry invalidation */
void	PPCMMU::tlbie()
{
	MMUTRACE("TLBIE\n");
	/* FIXME: Implement specific invalidation */
	dutlbInv();
	iutlbInv();
	generation_count++;
}

static __attribute__((unused)) void dump_htab(Bus *bus, PA htab, u32 mask)
{
	int topbit = m_fls(mask);
	int ptegs = 1<<(topbit+10);
	// FIXME: ROUND htab base!
	printf("---- HTAB at 0x%08x is %dKB\n", htab, 1<<(topbit+16-10));

	bool skipped_some = 0;

	for (int pteg = 0; pteg < ptegs; pteg++) {
		u32 pteg_contents[16];
		bool all_zero = true;
		for (int i = 0; i < 16; i++) {
			pteg_contents[i] = BSWAP32(bus->read32(htab + (pteg*64) + (i*4))); // FIXME unconditional
			all_zero = all_zero && (pteg_contents[i] == 0);
		}
		if (all_zero) {
			skipped_some = true;
		} else {
			if (skipped_some) {
				printf("PTEG ...");
				skipped_some = false;
			}
			printf("PTEG[%05d] %08x  ", pteg, htab + (pteg*64));
			for (int pte = 0; pte < 8; pte++) {
				u32 ptel = pteg_contents[pte*2 + 0];
				u32 pteh = pteg_contents[pte*2 + 1];
				if (ptel == 0 && pteh == 0)
					printf("        :        , ");
				else
					printf("%08x:%08x, ", ptel, pteh);
			}
			printf("\n");
		}
	}
	printf("----\n");
}

/* Translate an address from EA (er, VA) to PA.
 * This might access BATs or HTAB, depending on whether translation is enabled or not.
 * The translation context, in terms of I/D priv/unpriv, is provided and the
 * returned PA/permissions correspond to that context only.
 *
 * RnW is passed in, so that we can update the C bit correctly.
 *
 * On success, returns true.
 *
 * If translation fails this fn returns false and a fault_t.
 *
 *
 * Todo: Is this used just to fill an interpreter TLB, i.e. should it turn addr
 * into a tuple of PA, permissions etc.?  Or, is it used inline (ever?
 * e.g. debug?) in which case it should be passed RnW etc.?
 *
 * I think the best thing to do is simply pass ForInstOrData (which is known
 * from the type of TLB being filled) then return properties for that address.
 * The caller then checks the permissions (or stashes into a TLB, etc.)
 */
bool	PPCMMU::translateEA(VA addr, bool InD, bool RnW, bool priv,
			    PA *output_addr, mmuperms_t *perms, fault_t *fault_type)
{
	if ((InD && !enabled_i) ||
	    (!InD && !enabled_d)) {
                PPCMMU_PERMS_ANY(perms);
		*output_addr = addr;
		utlbInsert(InD, priv, addr, addr, perms->field);
		return true;
	}

	/*
	 * From PEM 2.3, 7.4.2:
	 * "Block address translation is enabled only when address translation
	 *  is enabled (MSR[IR] = 1 and/or MSR[DR] = 1). Also, a matching BAT
	 *  array entry always takes precedence over any segment descriptor
	 *  translation, independent of the setting of the SR[T] bit, and the
	 *  segment descriptor information is completely ignored."
	 */

	/* First, check the BATs to see if they match: */
	if (matchBAT(addr, InD, priv, output_addr, perms)) {
		if (!perms->r && !perms->w) {
			/* Even if there's a match, it might not have any useful
			 * perms, so it's a fault.  TODO: Does this need to be
			 * differentiated in DSISR with respect to a perms fault
			 * for SLB or page?  (I don't think so, they all set
			 * DSISR[4].)
			 */
			*fault_type = FAULT_PERMS;
			return false;
		}

		utlbInsert(InD, priv, addr, *output_addr, perms->field);
		return true;
	}

	/* No BAT hit.	Look up segments and possibly HTAB.
	 * "When an access uses the page or direct-store interface address
	 *  translation, the appropriate segment descriptor is required. In
	 *  32-bit implementations, one of the 16 on-chip segment registers
	 *  (which contain segment descriptors) is selected by the four
	 *  highest-order effective address bits."
	 */
	seg_t	*sr = &segreg[(addr >> 28) & 0xf];
	MMUTRACE("Segment lookup %d for EA %08x (%s, %s): ks %d, kp %d, n %d, vsid %08x\n",
		 (addr >> 28) & 0xf, addr, InD ? "inst" : "data", priv ? "priv" : "unpriv",
                 sr->ks, sr->kp, sr->n, sr->vsid);

	if (InD && sr->n) {
		*fault_type = FAULT_PERMS_NX;
		return false;
	}

	/* Generate VA: */
	u32	pgidx = (addr >> 12) & 0xffff;
	u64	va = (((u64)sr->vsid) << 16) | pgidx;
	u32	api = pgidx >> 10;

	MMUTRACE("Hash lookup: va 0x%08x (vsid %08x), pgidx 0x%08x (api 0x%02x)\n",
		 va, sr->vsid, pgidx, api);
	COUNT(CTR_MEM_HASH_LOOKUP);

	// Now, have VA:
	// - Could check VA in an internal TLB (as opposed to the interpreter TLB, which is flat like an ERAT)
	// - If no TLB entry, fetch from SDR1 HTAB

	u32	hashfn = (sr->vsid & 0x7ffff) ^ pgidx; /* 19 bits */
	u32	want_h = 0;

	for (int hash_lookup = 0; hash_lookup < 2; hash_lookup++) {
		PA	pri_pteg_addr = htab_phys | ((hashfn << 6) & ((htab_mask << 16) | 0xffc0));

		MMUTRACE("Hash lookup #%d at %p, htab %p, fn 0x%08x\n",
			 hash_lookup, pri_pteg_addr, htab_phys, hashfn);

		for (int pte = 0; pte < 8; pte++) {
			PA pte_pa = pri_pteg_addr + pte*8;
			/* FIXME: Optimise me-- I want a "get sim addr for.." and read all 64B! */
			u32 ptel = BS32(bus->read32(pte_pa));

			MMUTRACE("PTE[%d] = %08x, v %d, h %d, vsid 0x%08x, api %04x\n",
				 pte, ptel, !!(ptel & PPC_PTE_V), !!(ptel & PPC_PTE_H),
				 (ptel & 0x7fffff80) >> 7, ptel & 0x3f);

			if ((ptel & PPC_PTE_V) && // valid
			    ((ptel & PPC_PTE_H) == want_h) && // H 0 for primary, 1 for secondary
			    ((ptel & 0x7fffff80) == (sr->vsid << 7)) && // VSID matches
			    ((ptel & 0x3f) == api)) // remaining bits of pgidx match
			{
				/* OK, so read second half */
				u32 pteh = BS32(bus->read32(pte_pa+4));

				bool r = !!(pteh & 0x100);
				bool c = !!(pteh & 0x80);
				int wimg = (pteh & 0x78) >> 3;
				int pp = (pteh & 3);
				u32 rpn = pteh & 0xfffff000;
                                bool uw = ((!sr->kp && pp < 3) || (sr->kp && pp == 2));
                                bool kw = ((!sr->ks && pp < 3) || (sr->ks && pp == 2));
                                bool update_r = !r;
                                bool update_c = false;

                                if (priv) {
                                        perms->r = !(sr->ks && pp == 0);
                                        perms->w = kw
#ifndef UPDATE_RC
                                                && c
#endif
                                                ;
                                } else {
                                        perms->r = !(sr->kp && pp == 0);
                                        perms->w = uw
#ifndef UPDATE_RC
                                                && c
#endif
                                                ;
                                }

				COUNT(CTR_MEM_HASH_HIT);

				MMUTRACE("Hit in pte %d:  RPN 0x%08x, r %d, c %d, wimg 0x%x, pp %d\n",
					 pte, rpn, r, c, wimg, pp);

#ifdef UPDATE_RC
                                // OK, if it's not writable only because it's not C=1 then update C:
                                if ((!RnW && priv && kw && !c) ||
                                    (!RnW && !priv && uw && !c)) {
                                        update_c = true;
                                        c = true;
                                }
                                if (update_r || update_c) {
                                        pteh |= update_c ? 0x180 :
                                                update_r ? 0x100 : 0;
                                        MMUTRACE("Hit in pte %d: updating, new pteh %08x (R=1 C=%d)\n",
                                                 pte, pteh, !!update_c);
                                        bus->write32(pte_pa+4, BS32(pteh));
                                }
                                perms->clean = !c; // If it's NOT a write, stash C because it might later be written!
#else
                                perms->clean = 0;
#endif

				*output_addr = rpn | (addr & 0xfff);

				utlbInsert(InD, priv, addr, *output_addr, perms->field);

				return true;
			}
		}
		// Lookup missed.  Going round once more, use secondary hash:
		hashfn = ~hashfn;
		want_h = PPC_PTE_H;
	}

	MMUTRACE("translateEA failed (no page) for EA %08x\n", addr);
	*fault_type = FAULT_NO_PAGE;
	return false;
}

void	PPCMMU::setIRDR(bool ir, bool dr)
{
	enabled_d = dr;
	enabled_i = ir;
#ifdef SUPER_VERBOSE
	MMUTRACE("IR %d, DR %d\n", enabled_i, enabled_d);
#endif
}

/* Returns true if matching BAT found
 * However, even if match, it might still fault.  (Watch for perms being zero!)
 */
bool	PPCMMU::matchBAT(VA addr, bool InD, bool priv, PA *output_addr, mmuperms_t *perms)
{
	bat_t *bat_tab = InD ? ibat : dbat;

	for (int i = 0; i < PPCMMU_NR_BATS; i++) {
		bat_t *b = &bat_tab[i];
		VA amask = (0xfffe0000 << (b->bl_shift));

                if ((priv && !b->vs) || (!priv && !b->vp))
                        continue; // This entry doesn't match for this execution state

		if ((b->vp || b->vs) && ((addr ^ b->bepi) & amask) == 0) {
                        MMUTRACE("matchBAT: Hit BAT%d for addr %p (InD %d, priv %d)\n", i, addr, InD, priv);
			perms->field = 0;
			if (b->pp == 0) {
				MMUTRACE("matchBAT: Hit on addr %p, but no access\n",
					 addr);
				return true;
			}

			*output_addr = b->brpn | (addr & ~amask);

			// PP!=0, so Read-only, or Read-Write:
                        perms->r = 1;
			if (b->pp == 2)         // Read-write
                                perms->w = 1;

			return true;
		}
	}
        MMUTRACE("matchBAT: Miss for addr %p (InD %d)\n", addr, InD);
	return false;
}
