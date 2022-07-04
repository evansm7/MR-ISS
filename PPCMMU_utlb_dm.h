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

/* This file is included such that it is part of the PPCMMU class.
 *
 * Direct-mapped TLB
 *
 * This is intended for simulation performance, rather than to be an interesting
 * hardware layout.
 */

#define PPCMMU_UTLB_ENTRIES	128

utlb_t	pi_utlb[PPCMMU_UTLB_ENTRIES];
utlb_t	pd_utlb[PPCMMU_UTLB_ENTRIES];
utlb_t	ui_utlb[PPCMMU_UTLB_ENTRIES];
utlb_t	ud_utlb[PPCMMU_UTLB_ENTRIES];

#define PPCMMU_ADDR_TO_IDX(x)	(((x) >> 12) & (PPCMMU_UTLB_ENTRIES-1))

inline utlb_t *select_utlb(bool InD, bool priv)
{
        if (priv && InD)
                return pi_utlb;
        else if (priv && !InD)
                return pd_utlb;
        else if (!priv && InD)
                return ui_utlb;
        else // !priv && !InD
                return ud_utlb;
}

void	iutlbInv()
{
	memset(pi_utlb, 0, sizeof(utlb_t) * PPCMMU_UTLB_ENTRIES);
	memset(ui_utlb, 0, sizeof(utlb_t) * PPCMMU_UTLB_ENTRIES);
}

void	dutlbInv()
{
	memset(pd_utlb, 0, sizeof(utlb_t) * PPCMMU_UTLB_ENTRIES);
	memset(ud_utlb, 0, sizeof(utlb_t) * PPCMMU_UTLB_ENTRIES);
}

void	dumpUTLBs()
{
	for (int i = 0; i < PPCMMU_UTLB_ENTRIES; i++) {
		LOG("PI_UTLB[%02d]: V %d  EA %08x  PA %016lx  perms %02x\n",
		    i, pi_utlb[i].isValid(), pi_utlb[i].getEA(), pi_utlb[i].getPA(), pi_utlb[i].getPerms());
	}
	for (int i = 0; i < PPCMMU_UTLB_ENTRIES; i++) {
		LOG("PD_UTLB[%02d]: V %d  EA %08x  PA %016lx  perms %02x\n",
		    i, pd_utlb[i].isValid(), pd_utlb[i].getEA(), pd_utlb[i].getPA(), pd_utlb[i].getPerms());
	}
	for (int i = 0; i < PPCMMU_UTLB_ENTRIES; i++) {
		LOG("UI_UTLB[%02d]: V %d  EA %08x  PA %016lx  perms %02x\n",
		    i, ui_utlb[i].isValid(), ui_utlb[i].getEA(), ui_utlb[i].getPA(), ui_utlb[i].getPerms());
	}
	for (int i = 0; i < PPCMMU_UTLB_ENTRIES; i++) {
		LOG("UD_UTLB[%02d]: V %d  EA %08x  PA %016lx  perms %02x\n",
		    i, ud_utlb[i].isValid(), ud_utlb[i].getEA(), ud_utlb[i].getPA(), ud_utlb[i].getPerms());
	}
}

bool    utlbLookup(bool InD, bool priv, VA addr, u64 *output_addr, PPCMMU::mmuperms_t *perms)
{
        utlb_t *u = select_utlb(InD, priv);
	utlb_t *t = &u[PPCMMU_ADDR_TO_IDX(addr)];
	/* The address to match also has a bit to say it's valid, plus a bit to
	 * say whether it comes from IR/DR=1 context.
	 */
	u32 find_ea = (addr & ~0xfff) | PPCMMU_UTLB_VALID |
		((InD ? enabled_i : enabled_d) ? PPCMMU_UTLB_TR : 0);

	if (t->getEA() == find_ea) {
		*output_addr = t->getPA() | (addr & 0xfff);
		perms->field = t->getPerms();
		COUNT(CTR_MEM_UTLB_HIT);
		return true;
	}
	/* Miss. */
	COUNT(CTR_MEM_UTLB_MISS);
	return false;
}

/* This takes a PA (PPC) address to map to, and possibly converts it into a host
 * VA for a direct mmap access.
 */
void    utlbInsert(bool InD, bool priv, VA addr, PA out_addr, u32 perms)
{
        utlb_t *u = select_utlb(InD, priv);
        utlb_t *t = &u[PPCMMU_ADDR_TO_IDX(addr)];
	u64 pa;
	void *host_addr;
	if (bus->get_direct_map(out_addr, &host_addr)) {
		/* This can be dereferenced directly in loadXX/storeXX
		 * It doesn't get the top bit set.
		 */
		pa = (u64)host_addr;
	} else {
		/* There wasn't a direct mapping available; it was probably an
		 * IO device.  Setting the PPCMMU_HVA_IO_BIT flag causes a plain
		 * ol' access, e.g. bus->loadXX().
		 */
		pa = out_addr | PPCMMU_HVA_IO_BIT;
	}
	if (InD)
		t->set(addr, enabled_i ? PPCMMU_UTLB_TR : 0, pa, perms);
	else
		t->set(addr, enabled_d ? PPCMMU_UTLB_TR : 0, pa, perms);
}
