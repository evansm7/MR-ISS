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
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "DevSDCard.h"
#include "log.h"



#define SDCDBG          IOTRACE
#define SDCTRACE        IOTRACE
#define SDCWARN         WARN

#define VERBOSE

const uint8_t 	crc7_tab[] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e,//0
	0x90, 0x82, 0xb4, 0xa6, 0xd8, 0xca, 0xfc, 0xee,
	0x32, 0x20, 0x16, 0x04, 0x7a, 0x68, 0x5e, 0x4c,
	0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc,
	0x64, 0x76, 0x40, 0x52, 0x2c, 0x3e, 0x08, 0x1a,//4
	0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a,
	0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28,
	0xc6, 0xd4, 0xe2, 0xf0, 0x8e, 0x9c, 0xaa, 0xb8,
	0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6,//8
	0x58, 0x4a, 0x7c, 0x6e, 0x10, 0x02, 0x34, 0x26,
	0xfa, 0xe8, 0xde, 0xcc, 0xb2, 0xa0, 0x96, 0x84,
	0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06, 0x14,
	0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2,//12
	0x3c, 0x2e, 0x18, 0x0a, 0x74, 0x66, 0x50, 0x42,
	0x9e, 0x8c, 0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0,
	0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70,//15
	0x82, 0x90, 0xa6, 0xb4, 0xca, 0xd8, 0xee, 0xfc,//16
	0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c,
	0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce,
	0x20, 0x32, 0x04, 0x16, 0x68, 0x7a, 0x4c, 0x5e,
	0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98,//20
	0x76, 0x64, 0x52, 0x40, 0x3e, 0x2c, 0x1a, 0x08,
	0xd4, 0xc6, 0xf0, 0xe2, 0x9c, 0x8e, 0xb8, 0xaa,
	0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e, 0x28, 0x3a,
	0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34,//24
	0xda, 0xc8, 0xfe, 0xec, 0x92, 0x80, 0xb6, 0xa4,
	0x78, 0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06,
	0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96,
	0x2e, 0x3c, 0x0a, 0x18, 0x66, 0x74, 0x42, 0x50,//28
	0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0,
	0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70, 0x62,
	0x8c, 0x9e, 0xa8, 0xba, 0xc4, 0xd6, 0xe0, 0xf2 //31
};

// CRC7 on a 40b payload (for commands/responses)
// Note return val is pre-shifted << 1
static uint8_t 	sd_crc7(uint64_t v)
{
	uint8_t c = 0;
	for (int i = 0; i < 5; i++) {
		uint8_t d = (v >> 32) & 0xff;
		v <<= 8;
		c = crc7_tab[c ^ d];
	}
	return c;
}

// Same but for 120b
static uint8_t 	sd_crc7_120(uint8_t *v)
{
	uint8_t c = 0;
	for (int i = 0; i < 15; i++) {
		uint8_t d = v[14-i];
		c = crc7_tab[c ^ d];
	}
	return c;
}

bool    DevSDCard::init(char *filename)
{
        struct stat sb;

        /* Reset state */
        state = SD_STATE_IDLE;
        width = 1;
        acmd_pending = false;
        last_cmd = 0;
        last_arg = 0;
        offset = 0;
        block_count = 0;

        if (filename) {
                SDCTRACE("%s: opening '%s': ", __FUNCTION__, filename);
                fd = open(filename, O_RDWR);
                if (fd < 0) {
                        perror("Can't open SD image");
                        return false;
                }

                fstat(fd, &sb);
                image_blocks = sb.st_size/512;
        } else {
                image_blocks = 0;
        }

        return true;
}

uint32_t        DevSDCard::getStatus()
{
        uint32_t status = 0;
        /* Turn object state into status field */

        status |= (state << 9);

        if (acmd_pending)       /* FIXME clear on read */
                status |= (1 << 5);

        status |= (1 << 8);     /* READY_FOR_DATA */

        return status;
}

/* Oh. these are wrong. the lowest byte is sent first (i.e. is MSB!)
 * need to be reversed.
 */
static void     makeCID(uint32_t *r)
{
        uint8_t *cid = (uint8_t *)r;

        cid[15] = 0x69;
        cid[14] = 'E';
        cid[13] = 'M';

        cid[12] = 'M';
        cid[11] = 'y';
        cid[10] = 'S';
        cid[9] = 'D';
        cid[8] = ' ';

        cid[7] = 123;
        cid[6] = 'a';
        cid[5] = 'b';
        cid[4] = 'c';
        cid[3] = 'd';

        cid[2] = 0x01;
        cid[1] = 0x55;
        cid[0] = 0;
        uint8_t c = sd_crc7_120(&cid[1]) | 1;
        cid[0] = c;
}

static void     makeCSD(uint32_t *r, unsigned int blocks)
{
        uint8_t *csd = (uint8_t *)r;
        unsigned int s = (blocks/1024) - 1;   /* Units of 512KB */

        // v2
        csd[15] = 0x40; // CSD_STRUCTURE
        csd[14] = 0x0e; // TAAC
        csd[13] = 0;    // NSAC
        csd[12] = 0x0b; // TRAN_SPEED

        csd[11] = 0x5b; // CCC and READ_BL_LEN
        csd[10] = 0x59;

        csd[9] = 0x00;  // All sorts

        csd[8] = (s & 0xff0000) >> 16;  // C_SIZE
        csd[7] = (s & 0xff00) >> 8;
        csd[6] = s & 0xff;

        csd[5] = 0x7f;  // ERASE_BLK_LEN/SECTOR_SIZE
        csd[4] = 0x80;  // WR_GRP_SIZE

        csd[3] = 0x0a;  // R2W, WR_BL_LEN etc.
        csd[2] = 0x40;  // WR_BL_LEN
        csd[1] = 0x00;  // Various flags, like write protect/copy
        csd[0] = sd_crc7_120(&csd[1]) | 1;  // CRC-ish
}

/* FIXME: insert CRC, except for R3 (all-ones) */
static void     makeResponse(uint32_t cmd, uint32_t *r, uint32_t payload)
{
        uint64_t content = (((uint64_t)cmd & 0x3f) << 32) |
		payload | // 38b
		(0x0LL<<38); // 40b (w/ start bit 0, transmission bit 0)
	uint8_t crc = sd_crc7(content); // CRC includes start bit, hrrmm
        content = (content << 8) | crc | 1;

        r[0] = content & 0xffffffff;
        r[1] = content >> 32;
}

static void     checkRCA(uint32_t a)
{
        if ((a >> 16) != 0xbeef) {
                SDCWARN("[Strange RCA 0x%04x]\n", a >> 16);
        }
}

/* Main command execution interface:
 * Returns true if command was accepted/known, false if
 * timeout.
 *
 * This does not fully model SD card states or anything fancy --
 * only just enough to get data read/written.
 *
 * response array: byte 0 contains LSB.
 *
 * FIXME: error injection?
 */
bool    DevSDCard::command(uint32_t cmd, uint32_t arg, uint32_t *response)
{
        int r_type = -1;
        int subtype = -1;

        // FIXME: Check CRC

        SDCTRACE("[%s: SD cmd %d, arg %08x]\n", __FUNCTION__, cmd, arg);

        if (fd == -1)   /* No card */
                return false;

        last_cmd = cmd;
        last_arg = arg;
        last_acmd = acmd_pending;

        if (!acmd_pending) {
                switch (cmd) {
                case RCMD(CMD0):        /* GO_IDLE_STATE */
                        // Good cmd, but no response.
                        r_type = RRESP(CMD0);
                        state = SD_STATE_IDLE;
                        break;

                case RCMD(CMD2):        /* ALL_SEND_CID */
                        r_type = RRESP(CMD2);
                        subtype = 0;
                        state = SD_STATE_IDENT;
                        break;

                case RCMD(CMD3):        /* SEND_RELATIVE_ADDR */
                        r_type = RRESP(CMD3);
                        state = SD_STATE_STBY;
                        break;

                case RCMD(CMD8):        /* SEND_IF_COND */
                        r_type = RRESP(CMD8);
                        break;

                case RCMD(CMD6):        /* SWITCH_FUNCTION */
                        /* Ignore argument, unimportant */
                        SDCTRACE("SD: switch arg %08x\n", arg);
                        r_type = RRESP(CMD6);
                        state = SD_STATE_DATA;
                        break;

                case RCMD(CMD7):        /* SELECT_DESELECT_CARD */
                        checkRCA(arg);
                        r_type = RRESP(CMD7);
                        state = SD_STATE_TRAN;
                        break;

                case RCMD(CMD9):        /* SEND_CSD */
                        checkRCA(arg);
                        r_type = RRESP(CMD9);
                        subtype = 1;
                        break;

                case RCMD(CMD12):       /* STOP_TRANSMISSION */
                        r_type = RRESP(CMD12);
                        /* State -> TRAN? */
                        state = SD_STATE_TRAN;
                        break;

                case RCMD(CMD13):       /* SEND_STATUS */
                        r_type = RRESP(CMD13);
                        break;

                case RCMD(CMD17):       /* READ_SINGLE_BLOCK */
                        r_type = RRESP(CMD17);
                        offset = arg * 512;
                        state = SD_STATE_DATA;
                        break;

                case RCMD(CMD18):       /* READ_MULTIPLE_BLOCK */
                        r_type = RRESP(CMD18);
                        offset = arg * 512;
                        state = SD_STATE_DATA;
                        break;

                case RCMD(CMD23):       /* SET_BLOCK_COUNT */
                        r_type = RRESP(CMD23);
                        block_count = arg;
                        /* Technically this should 'stick' for the next
                         * command only!
                         */
                        break;

                case RCMD(CMD24):       /* WRITE_BLOCK */
                        r_type = RRESP(CMD24);
                        offset = arg * 512;
                        state = SD_STATE_DATA;
                        break;

                case RCMD(CMD25):       /* WRITE_MULTIPLE_BLOCK */
                        r_type = RRESP(CMD25);
                        offset = arg * 512;
                        state = SD_STATE_DATA;
                        break;

                case RCMD(CMD55):       /* APP_CMD */
                        r_type = RRESP(CMD55);
                        acmd_pending = true;
                        break;

                default:
                        SDCWARN("[%s: Unknown command %d (arg %08x)]\n",
                                __FUNCTION__, cmd, arg);
                }
        } else {
                switch (cmd) {
                case RCMD(ACMD41):      /* SD_SEND_OP_COND */
                        r_type = RRESP(ACMD41);
                        state = SD_STATE_READY;
                        break;

                case RCMD(ACMD6):       /* SET_BUS_WIDTH */
                        r_type = RRESP(ACMD6);
                        if (!(arg & 1)) {
                                if (arg & 2)
                                        width = 4;
                                else
                                        width = 1;
                                SDCTRACE("SD: Width set to %d\n", width);
                        } else {
                                SDCTRACE("SD: Weird ACMD6 arg %08x\n", arg);
                        }
                        break;

                case RCMD(ACMD51):      /* SEND_SCR */
                        r_type = RRESP(ACMD51);
                        state = SD_STATE_DATA;
                        break;

                case RCMD(ACMD13):      /* SD_STATUS */
                        r_type = RRESP(ACMD13);
                        state = SD_STATE_DATA;
                        break;

                default:
                        SDCWARN("[%s: Unknown ACMD %d (arg %08x)]\n",
                                __FUNCTION__, cmd, arg);
                }
                acmd_pending = false;
        }

        if (r_type <= 0)
                return false;

        /* Deal with response type */
        switch (r_type) {
        case 0xf:
        case 1:         /* Return status */
                makeResponse(cmd, response, getStatus()); /* FIXME CRC */
                break;

        case 2:
                if (subtype == 0) { /* CID */
                        makeCID(response);
                } else if (subtype == 1) { /* CSD */
                        makeCSD(response, image_blocks);
                } else {
                        FATAL("%s: Bad R2 subtype %d\n", __FUNCTION__,
                              subtype);
                }
                break;

        case 3:         /* Return OCR */
                makeResponse(cmd, response, 0x80000000 | /* Not busy */
                             0x40000000 | /* CCS=1, SDHC */
                             0x00ff8000); /* All the volts */
                break;

        case 6:         /* RCA */
                makeResponse(cmd, response, 0xbeef0000);
                break;

        case 7:         /* Return card interface condition. */
                makeResponse(cmd, response, arg & 0xfff);
                break;

        default:
                FATAL("%s: Unhandled response type %d\n",
                      __FUNCTION__, r_type);
        }

        return true;
}

/* SCR and status take the "wide data" format, where MSB is sent first
 * (so lands in the lowest byte in memory)
 */
static void     writeSCR(uint8_t *d)
{
        d[0] = 0x02;    /* SD_SPEC v2 */
        d[1] = 0x05;    /* bus widths 1 & 4 */
        d[2] = 0x80;    /* SD_SPEC3 = 1 */
        d[3] = 0x02;    /* CMD23 OK */
        d[4] = d[5] = d[6] = d[7] = 0;
}

static void     writeStatus(uint8_t *d, int width)
{
        /* Remember byte 0 = MSB! */

        memset(d, 0, 64);

        /* 511:504 */
        d[0] = (width == 4) ? 0x80 : 0x00;
        /* 503:496 */
        d[1] = 0;
        /* 495:488, :480 */
        d[2] = 0;       /* Type = SD */
        d[3] = 0;
        /* 479:472, :464, :456, :448 */
        d[4] = 0;       /* SIZE_OF_PROTECTED_AREA */
        d[5] = 0;
        d[6] = 0;
        d[7] = 0;
        /* 447:440 */
        d[8] = 0x04;    /* SPEED_CLASS */
        /* 439:432 */
        d[9] = 0xff;    /* PERFORMANCE_MOVE */
        /* 431:424 */
        d[10] = 0x10;   /* AU_SIZE */
        /* 423:416, :408 */
        d[11] = 0;      /* ERASE_SIZE */
        d[12] = 0;
        /* 407:400 */
        d[13] = 0;      /* ERASE_TIMEOUT + ERASE_OFFSET */
        /* 399:392 */
        d[14] = 0;      /* UHS_SPEED_GRADE + UHS_AU_SIZE */
}

static void     writeSwitchStatus(uint8_t *d)
{
        /* Remember byte 0 = MSB! */

        memset(d, 0, 64);
        d[1] = 2;       /* 2ma in 511:496 */
}

/* Returns true & data from a prior read command, or false if
 * the request is unexpected/somehow bad.
 * Size (in bytes) should match that of the command(s).
 */
bool    DevSDCard::read(uint8_t *data_out, unsigned int size, unsigned int *err)
{
        /* 1. Are we in DATA state?  A previous command should've put
         * us there.
         *
         * (It's OK if not though, SW can set an RX pending then perform
         * a sequence of commands that result in a read.)
         */
        if (state != SD_STATE_DATA) {
                *err = 0;       /* But not an error */
                return false;
        }
        SDCDBG("[SD read: last cmd %d (acmd %d), size %d, offset %d]\n",
               last_cmd, last_acmd, size, offset);

        /* 2. What was the command that got us there? */
        if (last_acmd && last_cmd == RCMD(ACMD51)) {
                if (size != 8) {
                        SDCWARN("[%s: Odd size %d for SEND_SCR!]\n", __FUNCTION__,
                                size);
                }
                writeSCR(data_out);
        } else if (last_acmd && last_cmd == RCMD(ACMD13)) {
                if (size != 64) {
                        SDCWARN("[%s: Odd size %d for SD_STATUS!]\n", __FUNCTION__,
                                size);
                }
                writeStatus(data_out, width);
        } else if (!last_acmd && last_cmd == RCMD(CMD6)) {
                if (size != 64) {
                        SDCWARN("[%s: Odd size %d for CMD6!]\n", __FUNCTION__,
                                size);
                }
                writeSwitchStatus(data_out);
        } else if (!last_acmd && last_cmd == RCMD(CMD17)) {
                /* Read single block */
                SDCTRACE("[SD read block, offs %d\n", offset);
                if (size != 512) {
                        SDCWARN("[%s: Odd size %d for CMD17!]\n", __FUNCTION__,
                                size);
                }

                ssize_t r = pread(fd, data_out, 512, offset);

                if (r != 512) {
                        if (r < 0) {
                                perror("SDread pread: ");
                        }
                        SDCWARN("[%s: Can't read, ret %d :(]\n", __FUNCTION__, r);
                        *err = 1;
                        state = SD_STATE_TRAN;
                        return false;
                }
        } else if (!last_acmd && last_cmd == RCMD(CMD18)) {
                SDCTRACE("[SD read %d blocks, offs %d, size %d]\n",
                         block_count, offset, size);
                /* Read multiple blocks */

                /* SD cards can be asked to read continuously;
                 * if the block count hasn't been set then fake the
                 * "stream until CMD12" behaviour by reading exactly as
                 * much as the host controller asks for (with the rationale
                 * that it will have been asked to DMA a certain amount).
                 */
                if (block_count && size != block_count*512) {
                        SDCWARN("[%s: Odd size %d for CMD18 (block_count %d)!]\n",
                                __FUNCTION__, size, block_count);
                }

                ssize_t r = pread(fd, data_out, size, offset);

                if (r != size) {
                        if (r < 0) {
                                perror("SDread pread: ");
                        }
                        SDCWARN("[%s: Can't read, ret %d :(]\n", __FUNCTION__, r);
                        *err = 1;
                        state = SD_STATE_TRAN;
                        return false;
                }
        } else {
                SDCWARN("[%s: Not sure what to do (prev cmd %d, acmd %d) :(]\n",
                        __FUNCTION__, last_cmd, last_acmd);
                *err = 1;
                state = SD_STATE_TRAN;
                return false;
        }

        state = SD_STATE_TRAN;

        return true;
}

/* Returns true if write of data succeeded, or false if
 * the request is unexpected/bad.
 */
bool    DevSDCard::write(uint8_t *data_in, unsigned int size, unsigned int *err)
{
        if (state != SD_STATE_DATA) {
                *err = 0;       /* But not an error */
                SDCWARN("[SD write: state %d]\n", state);
                return false;
        }
        SDCDBG("[SD write: last cmd %d (acmd %d), size %d, offset %d]\n",
               last_cmd, last_acmd, size, offset);

        if (!last_acmd && last_cmd == RCMD(CMD24)) {
                /* Write single block */
                SDCTRACE("[SD write block, offs %d\n", offset);
                if (size != 512) {
                        SDCWARN("[%s: Odd size %d for CMD24!]\n", __FUNCTION__,
                                size);
                }

                ssize_t r = pwrite(fd, data_in, 512, offset);

                if (r != 512) {
                        if (r < 0) {
                                perror("SDwrite pwrite: ");
                        }
                        SDCWARN("[%s: Can't write, ret %d :(]\n", __FUNCTION__, r);
                        *err = 1;
                        state = SD_STATE_TRAN;
                        return false;
                }
        } else if (!last_acmd && last_cmd == RCMD(CMD25)) {
                SDCTRACE("[SD write %d blocks, offs %d, size %d\n",
                         block_count, offset, size);
                /* Write multiple blocks */

                /* Similarly to the read multiple case, fake the
                 * continuous-write behaviour by writing exactly what
                 * the host controller asks.
                 */
                if (block_count && size != block_count*512) {
                        SDCWARN("[%s: Odd size %d for CMD25 (block_count %d)!]\n",
                                __FUNCTION__, size, block_count);
                }

                ssize_t r = pwrite(fd, data_in, size, offset);

                if (r != size) {
                        if (r < 0) {
                                perror("SDwrite pwrite: ");
                        }
                        SDCWARN("[%s: Can't write, ret %d :(]\n", __FUNCTION__, r);
                        *err = 1;
                        state = SD_STATE_TRAN;
                        return false;
                }
        } else {
                SDCWARN("[%s: Not sure what to do (prev cmd %d, acmd %d) :(]\n",
                        __FUNCTION__, last_cmd, last_acmd);
                *err = 1;
                state = SD_STATE_TRAN;
                return false;
        }

        state = SD_STATE_TRAN;
        return true;
}
