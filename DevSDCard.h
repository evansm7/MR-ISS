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

#ifndef DEVSDCARD_H
#define DEVSDCARD_H

#include <inttypes.h>

/* Commands encode their response type in 2nd byte:
 * 0 = no response, f = R1b, otherwise Rx
 */
#define CMD0	0x000 	// GO_IDLE_STATE	-
#define CMD2	0x202	// ALL_SEND_CID		R2, 136b
#define CMD3	0x603	// SEND_RELATIVE_ADDR	R6
#define CMD6    0x106   // SWITCH_FUNCTION      R1, 512b data status
#define CMD7	0xf07	// SELECT_DESELECT_CARD	R1b
#define CMD8	0x708	// SEND_IF_COND		R7
#define CMD9	0x209	// SEND_CSD		R2, 136b
#define CMD10	0x20a	// SEND_CID		R2, 136b
#define CMD12   0xf0c   // STOP_TRANSMISSION    R1b
#define CMD13	0x10d	// SEND_STATUS		R1
#define CMD17	0x111	// READ_SINGLE_BLOCK	R1, 4096b data response
#define CMD18   0x112   // READ_MULTIPLE_BLOCK  R1
#define CMD23   0x117   // SET_BLOCK_COUNT      R1
#define CMD24	0x118	// WRITE_BLOCK		R1
#define CMD25	0x119	// WRITE_MULTIPLE_BLOCK R1
#define CMD55	0x137	// APP_CMD		R1
#define ACMD13	0x10d	// SD_STATUS    	R1, 512b data
#define ACMD41	0x329	// SD_SEND_OP_COND	R3
#define ACMD51  0x133   // SEND_SCR             R1, 64b data
#define ACMD6	0x106	// SET_BUS_WIDTH	R1

// Somewhat clearer .. strip out the raw command value from the #defines above:
#define RCMD(x)	((uint8_t)((x) & 0xff))
#define RRESP(x) ((uint8_t)(((x) >> 8) & 0xf))

// Lengths encoded as: 0 = none, 2 = 48b, 3 = 136b
static const uint8_t resp_type_to_resp_len[16] = {
        0, 2, 3, 2,  0, 0, 2, 2,
        0, 0, 0, 0,  0, 0, 0, 2
};


#define SD_STATE_IDLE   0
#define SD_STATE_READY  1
#define SD_STATE_IDENT  2
#define SD_STATE_STBY   3
#define SD_STATE_TRAN   4
#define SD_STATE_DATA   5
#define SD_STATE_RCV    6
#define SD_STATE_PRG    7
#define SD_STATE_DIS    8

class DevSDCard {
public:
        DevSDCard() : fd(-1),
                image_blocks(0),
                acmd_pending(false),
                state(SD_STATE_IDLE),
                width(1)
        {}

        bool    init(char *filename);

        /* Main command execution interface:
         * Returns true if command was accepted/known, false if
         * timeout.
         */
        bool    command(uint32_t cmd, uint32_t arg, uint32_t *response);

        /* Returns true & data from a prior read command, or false if
         * the request is unexpected/somehow bad.
         * Size (in bytes) should match that of the command(s).
         */
        bool    read(uint8_t *data_out, unsigned int size, unsigned int *err);

        /* Returns true if write of data succeeded, or false if
         * the request is unexpected/bad.
         */
        bool    write(uint8_t *data_in, unsigned int size, unsigned int *err);

private:
        uint32_t        getStatus();

        int fd;
        unsigned int image_blocks;
        bool acmd_pending;
        int state;
        int width;

        uint32_t last_cmd;
        uint32_t last_arg;
        bool last_acmd;
        uint32_t offset;
        uint32_t block_count;
};

#endif
