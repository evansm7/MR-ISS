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

/* DevSD
 *
 * Emulation of the MattRISC platform's SD host controller
 *
 * ME 11 April 2022
 */

#include <stdio.h>
#include "DevSD.h"

#define SD_REG_RB0	0
#define SD_REG_RB1	1
#define SD_REG_RB2	2
#define SD_REG_RB3	3
#define SD_REG_CB0	4
#define SD_REG_CB1	5
#define SD_REG_CTRL	6
#define         SD_CTRL_CMDREQ          0x01
#define         SD_CTRL_RESP_SHIFT      2
#define         SD_CTRL_RESP_MASK       0x0c
#define         SD_CTRL_RESP_NONE       0
#define         SD_CTRL_RESP_48         2
#define         SD_CTRL_RESP_136        3
#define         SD_CTRL_RXREQ           0x10
#define         SD_CTRL_TXREQ           0x100
#define         SD_CTRL_CLKDIV_SHIFT    24
#define SD_REG_STATUS	7
#define         SD_STATUS_CMDACK        0x00000001
#define         SD_STATUS_CMD_PEND      0x00000002
#define         SD_STATUS_RXACK         0x00000010
#define         SD_STATUS_RX_PEND       0x00000020
#define         SD_STATUS_TXACK         0x00000100
#define         SD_STATUS_TX_PEND       0x00000200
#define         SD_STATUS_DMA_BUSY      0x00001000
#define         SD_STATUS_RX_IDLE       0x80000000
#define         SD_STATUS_CMD_SHIFT     2
#define         SD_STATUS_RX_SHIFT      6
#define         SD_STATUS_TX_SHIFT      10
#define         SD_STATUS_DMA_SHIFT     14
#define         SD_STATUS_CMD(x)        (((x) & 0xc) >> SD_STATUS_CMD_SHIFT)
#define         SD_STATUS_CMD_OK        0
#define         SD_STATUS_CMD_TIMEOUT   1
#define         SD_STATUS_CMD_BADEND    2
#define         SD_STATUS_CMD_BADCRC    3
#define         SD_STATUS_RX(x)         (((x) & 0xc0) >> SD_STATUS_RX_SHIFT)
#define         SD_STATUS_RX_OK         0
#define         SD_STATUS_RX_TIMEOUT    1
#define         SD_STATUS_RX_BADEND     2
#define         SD_STATUS_RX_BADCRC     3
#define         SD_STATUS_TX(x)         (((x) & 0xc00) >> SD_STATUS_TX_SHIFT)
#define         SD_STATUS_TX_OK         0
#define         SD_STATUS_TX_TIMEOUT    1
#define         SD_STATUS_TX_BADEND     2
#define         SD_STATUS_TX_BADCRC     3
#define         SD_STATUS_DMA(x)        (((x) & 0xc000) >> SD_STATUS_DMA_SHIFT)
#define         SD_STATUS_DMA_OK        0
#define         SD_STATUS_DMA_RX_OVF    1
#define SD_REG_STATUS2	8
#define         SD_STATUS2_TXBLOCKS(x)  ((x) & 0xffff)
#define SD_REG_DATACFG  12      // [7:0]=nrWordsInBlock, [31:16]=nrBlocksMinusOne
#define         SD_DATACFG_WIB(x)       ((x) & 0xff)
#define         SD_DATACFG_NRBLK(x)     (((x) & 0xffff0000) >> 16)
#define SD_REG_DMA_ADDR	14
#define SD_REG_IRQ      15
#define         SD_IRQ_RX_COMPLETE      0x01
#define         SD_IRQ_TX_COMPLETE      0x02
#define         SD_IRQ_RX_ENABLE        0x100
#define         SD_IRQ_TX_ENABLE        0x200


////////////////////////////////////////////////////////////////////////////////
// Register interface

/* Currently all the pending/ack stuff is zero-time, i.e.
 * a write that sets something pending will immediately
 * complete the work and set it non-pending.  Currently this is then pointless
 * -- except in future I want to add small delays, and this will be visible.
 */
bool    DevSD::cmdPending()
{
        return !!(regs[SD_REG_CTRL] & SD_CTRL_CMDREQ) ^ cmd_ack;
}

bool    DevSD::txPending()
{
        return !!(regs[SD_REG_CTRL] & SD_CTRL_TXREQ) ^ tx_ack;
}

bool    DevSD::rxPending()
{
        return !!(regs[SD_REG_CTRL] & SD_CTRL_RXREQ) ^ rx_ack;
}

u32	DevSD::read32(PA addr)
{
	u32 data = 0;
	int reg = (addr & 0x3f) >> 2;

	switch (reg) {
        case SD_REG_RB0:
        case SD_REG_RB1:
        case SD_REG_RB2:
        case SD_REG_RB3:
        case SD_REG_CB0:
        case SD_REG_CB1:
        case SD_REG_CTRL:
        case SD_REG_DATACFG:
        case SD_REG_DMA_ADDR:
                data = regs[reg];
                break;

        case SD_REG_STATUS: {
                data = (cmd_ack ? SD_STATUS_CMDACK : 0) |
                        (cmdPending() ? SD_STATUS_CMD_PEND : 0) |
                        (rx_ack ? SD_STATUS_RXACK : 0) |
                        (rxPending() ? SD_STATUS_RX_PEND : 0) |
                        (tx_ack ? SD_STATUS_TXACK : 0) |
                        (txPending() ? SD_STATUS_TX_PEND : 0) |
                        ((cmd_status & 3) << SD_STATUS_CMD_SHIFT) |
                        ((rx_status & 3) << SD_STATUS_RX_SHIFT) |
                        ((tx_status & 3) << SD_STATUS_TX_SHIFT) |
                        ((dma_status & 3) << SD_STATUS_DMA_SHIFT);
        } break;

        case SD_REG_STATUS2:
                data = txblocks & 0xffff;
                break;

        case SD_REG_IRQ:
                data = (irq_rx_triggered ? SD_IRQ_RX_COMPLETE : 0) |
                        (irq_tx_triggered ? SD_IRQ_TX_COMPLETE : 0) |
                        (irq_rx_enabled ? SD_IRQ_RX_ENABLE : 0) |
                        (irq_tx_enabled ? SD_IRQ_TX_ENABLE : 0);
                break;

        default:
                FATAL("DevSD: Read from undef reg, addr 0x%08x (reg %d)\n", addr, reg);
	}

	IOTRACE("DevSD: RD[%x] <= 0x%08x\n", addr, data);
	return data;
}

u16	DevSD::read16(PA addr)
{
	FATAL("DevSD::read16(%x)\n", addr);
	return 0;
}

u8	DevSD::read8(PA addr)
{
	FATAL("DevSD::read8(%x)\n", addr);
	return 0;
}

void	DevSD::write32(PA addr, u32 data)
{
	int reg = (addr & 0x3f) >> 2;

	IOTRACE("DevSD: WR[%x] => 0x%08x\n", addr, data);

	switch (reg) {
        case SD_REG_CB0:
        case SD_REG_CB1:
        case SD_REG_CTRL:
        case SD_REG_DATACFG:
        case SD_REG_DMA_ADDR:
                regs[reg] = data;
                break;

	case SD_REG_IRQ:
                irq_tx_enabled = !!(data & SD_IRQ_TX_ENABLE);
                irq_rx_enabled = !!(data & SD_IRQ_RX_ENABLE);
                if (data & SD_IRQ_TX_COMPLETE)
                        irq_tx_triggered = false;
                if (data & SD_IRQ_RX_COMPLETE)
                        irq_rx_triggered = false;
                break;

        default:
                FATAL("DevSD: Write of 0x%08x to undef reg, addr 0x%08x\n", data, addr);
	}

        /* Check for new work to do */
        checkForWork();
}

void	DevSD::write16(PA addr, u16 data)
{
	FATAL("DevSD::write16(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSD::write8(PA addr, u8 data)
{
	FATAL("DevSD::write8(" FMT_PA ", %04x)\n", addr, data);
}

void	DevSD::init(DevSDCard *sdc)
{
	/* Initialise registers */
	for (int i = 0; i < DEVSD_REG_END; i++) {
		regs[i] = 0;
	}

        txblocks = 0;
        irq_rx_triggered = irq_tx_triggered = false;
        irq_rx_enabled = irq_tx_enabled = false;

        cmd_ack = tx_ack = rx_ack = false;

        cmd_status = tx_status = rx_status = dma_status = 0;

        sdcard = sdc;
}

/* Periodic callback: Check for events and update the screen.
 */
void	DevSD::periodic(u64 ticks)
{
	return;
}

void 	*DevSD::getAddrDMA(PA addr)
{
	void *host_addr;
	if (!bus) {
		FATAL("DevSD::getAddrDMA: Needs bus setup\n");
	} else {
		if (bus->get_direct_map(addr, &host_addr)) {
			return host_addr;
		} else {
			FATAL("DevSD::getAddrDMA: Can't direct-map PA %08x\n", addr);
		}
	}
	return 0;
}

void    DevSD::checkForWork()
{
        if (cmdPending()) {
                uint32_t cmd = (regs[SD_REG_CB1] >> 8) & 0x3f;
                uint32_t arg = (regs[SD_REG_CB1] << 24) | (regs[SD_REG_CB0] >> 8);
                bool cmd_ok = sdcard->command(cmd, arg, &regs[SD_REG_RB0]);

                /* FIXME: Error injection of more interesting cases, such as
                 * missed end bits or CRC issues.  Or, just drop commands at
                 * random to simulate outgoing CRC problems.
                 */
                // printf("[cmd %d arg %08x, resp %08x:%08x:%08x:%08x]\n",
                //        cmd, arg, regs[SD_REG_RB0], regs[SD_REG_RB1],
                //        regs[SD_REG_RB2], regs[SD_REG_RB3]);

                cmd_status = cmd_ok ? SD_STATUS_CMD_OK : SD_STATUS_CMD_TIMEOUT;

                cmd_ack = !cmd_ack;

                /* RX is "triggered" off a command completion, i.e. doesn't
                 * do anything on its own.
                 */
                if (rxPending()) {
                        unsigned int rx_len;
                        if (SD_DATACFG_NRBLK(regs[SD_REG_DATACFG]) == 0)
                                rx_len = SD_DATACFG_WIB(regs[SD_REG_DATACFG])*4;
                        else
                                rx_len = (SD_DATACFG_NRBLK(regs[SD_REG_DATACFG])+1)*512;

                        uint8_t *base = (uint8_t *)getAddrDMA(regs[SD_REG_DMA_ADDR]);
                        unsigned int err = 0;
                        bool rx_ok = sdcard->read(base, rx_len, &err);

                        IOTRACE("DevSD: read to %p, %08x, %d, OK%d/err%d dcfg %08x\n",
                               base, regs[SD_REG_DMA_ADDR],
                               rx_len, rx_ok, err, regs[SD_REG_DATACFG]);
                        if (rx_ok || err) {
                                /* Mimic a t/o if the read was unexpected/bad in some way. */
                                rx_status = err ? SD_STATUS_RX_TIMEOUT : SD_STATUS_RX_OK;

                                rx_ack = !rx_ack;
                                irq_rx_triggered = true;
                        }
                }
        }

        if (txPending()) {
                unsigned int tx_len;
                unsigned int err = 0;
                tx_len = (SD_DATACFG_NRBLK(regs[SD_REG_DATACFG])+1)*512;
                uint8_t *base = (uint8_t *)getAddrDMA(regs[SD_REG_DMA_ADDR]);

                bool tx_ok = sdcard->write(base, tx_len, &err);
                IOTRACE("DevSD: write to %p, %08x, %d, OK%d/err%d dcfg %08x\n",
                        base, regs[SD_REG_DMA_ADDR],
                        tx_len, tx_ok, err, regs[SD_REG_DATACFG]);
                tx_status = tx_ok ? SD_STATUS_TX_OK : SD_STATUS_TX_TIMEOUT;

                tx_ack = !tx_ack;
                irq_tx_triggered = true;
        }

        if (intc) {
                if ((irq_tx_triggered && irq_tx_enabled) ||
                    (irq_rx_triggered && irq_rx_enabled)) {
                        IOTRACE("DevSD: triggering irq %d\n", irq_number);
                        intc->triggerIRQ(irq_number);
                } else {
                        // Hacky .. levels aren't supported by INTC yet :P
                }
        }

        //printf("\n ctrl %08x, rxack %d\n\n", regs[SD_REG_CTRL], rx_ack);
}
