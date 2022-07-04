# Makefile for MR-ISS test/demo
#
# Copyright 2016-2022 Matt Evans
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


CROSS_COMPILE ?= powerpc-linux-gnu-
CROSS_CC=$(CROSS_COMPILE)gcc
CROSS_LD=$(CROSS_COMPILE)ld
CROSS_OBJCOPY=$(CROSS_COMPILE)objcopy
CROSS_SIZE=$(CROSS_COMPILE)size

INCLUDES += -I.

CFLAGS=-mpowerpc -mcpu=604 -std=c99 -Os -g -ggdb $(INCLUDES) -ffreestanding -msoft-float -Wall
CFLAGS += -fdata-sections -ffunction-sections -fno-delete-null-pointer-checks -Wno-shift-overflow
LIBS=-lgcc
LINKFLAGS=-Wl,--gc-sections -nostdlib

LINKSCRIPT=ppc_fw.ld

DEMO_OBJECTS = head.o
DEMO_OBJECTS += gfx_fb_demo.o
DEMO_OBJECTS += uart.o
DEMO_OBJECTS += fx_plasma.o
DEMO_OBJECTS += lookuptables.o

all:	test.bin

clean:
	rm -f *.o *~ *.bin *.elf

%.bin:	%.elf
	$(CROSS_OBJCOPY) $< -O binary $@

test.elf: $(DEMO_OBJECTS)
	$(CROSS_CC) $(LINKFLAGS) -Wl,-T $(LINKSCRIPT) $^ $(LIBS) -o $@
	$(CROSS_SIZE) $@

%.o:	%.c
	$(CROSS_CC) $(CFLAGS) -c $< -o $@
%.o:	%.S
	$(CROSS_CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

