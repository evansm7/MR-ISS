# Makefile for MR-ISS project
#
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
#

################################################################################
# Build flags/tunables:

# If enabled (1), don't build MMU/address translation, treat memory as flat:
FLAT_MEM ?= 0
# Turn off compiler optimisation, add debug symbols etc:
DEBUG ?= 0
# Trap on divide instructions:
NO_DIVIDE ?= 0
# Trap syscalls, provide a trivial OS environment for basic I/O:
HOSTED ?= 0
# Build event counters:
COUNTERS ?= 1
# Enable tracing:
TRACING ?= 1
# Trivial JIT support:
JIT ?= 0
# Platform selection (see platform.{cc,h}):
PLATFORM ?= 3
# Alignment:
#  0 = strict, don't support any non-aligned memory ops
#  1 = Permit unaligned within a 64b boundary (like MR)
#  2 = Permit any alignment
ALIGNMENT ?= 1
# Don't use: short-circuits address translation
DUMMY_MEM_ACCESS ?= 0
# Don't try to use SDL for LCDC output (i.e. no LCD output):
NO_SDL ?= 0
# Use SPIDevGPIO, for real-world SPI devices:
GPIO ?= 0
# RC update: Linux works w/o update of PTE RC bits; UPDATE_RC=0 will improve sim performance a little:
UPDATE_RC ?= 1

################################################################################

CXXFLAGS = -fno-rtti
CXXFLAGS += -ggdb -g
CXXFLAGS += -Wall

ifeq ($(DEBUG), 0)
	CXXFLAGS += -O3
else
	CXXFLAGS += -O0
endif

ifeq ($(NO_DIVIDE), 1)
	CXXFLAGS += -DNO_DIVIDE
endif

ifeq ($(HOSTED), 1)
	CXXFLAGS += -DHOSTED
endif

ifeq ($(NO_SDL), 1)
	CXXFLAGS += -DNO_SDL
endif

ifeq ($(UPDATE_RC), 1)
	CXXFLAGS += -DUPDATE_RC
endif

CXXFLAGS += -DPLATFORM=$(PLATFORM)
CXXFLAGS += -DMEM_OP_ALIGNMENT=$(ALIGNMENT)
CXXFLAGS += -DDUMMY_MEM_ACCESS=$(DUMMY_MEM_ACCESS)

# Host-specific stuff
CMD_SYMBOLS=UNK
SYMBOL_PREFIX=UNK
ifeq ($(shell uname -s), Darwin)
	CMD_SYMBOLS=objdump -t
	SYMBOL_PREFIX=_
else
ifeq ($(shell uname -s), Linux)
	CMD_SYMBOLS=objdump -x
	SYMBOL_PREFIX=
endif
endif

LINK_FLAGS=
LINK_LIBS=-lpthread -lutil

SOURCES=main.cc log.cc Config.cc
SOURCES+=platform.cc
SOURCES+=stats.cc
SOURCES+=management.cc
SOURCES+=sim_state.cc
SOURCES+=PPCCPUState.cc
SOURCES+=PPCMMU.cc
SOURCES+=OS.cc
SOURCES+=DevUart.cc
SOURCES+=DevSimpleUart.cc
SOURCES+=DevDebug.cc
SOURCES+=DevDummy.cc
SOURCES+=DevXpsIntc.cc
SOURCES+=DevGPIO.cc
SOURCES+=SerialTCP.cc
SOURCES+=SerialPTY.cc
SOURCES+=DevSBD.cc
SOURCES+=DevSPI.cc
SOURCES+=DevSDCard.cc
SOURCES+=DevSD.cc

CORE_INTERP_SOURCES=PPCInterpreter.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Arithmetic.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Memory.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Branch.cc
CORE_INTERP_SOURCES+=PPCInterpreter_CondReg.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Logical.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Special.cc
CORE_INTERP_SOURCES+=PPCInterpreter_Trap.cc
CORE_INTERP_SOURCES+=PPCInterpreter_auto.cc

ifeq ($(JIT), 1)
	ASM_SOURCES+=op_addrs.S
	SOURCES+=runloop.cc
	SOURCES+=blockstore.cc
	SOURCES+=blockgen.cc
endif

ifeq ($(PLATFORM), 2)	# MR2
	SOURCES+=DevLCDC.cc
	ifeq ($(NO_SDL), 0)
		CXXFLAGS += `sdl2-config --cflags`
		LINK_LIBS += `sdl2-config --libs`
	endif
endif

ifeq ($(PLATFORM), 3)	# MR3
	SOURCES+=DevLCDC.cc
	ifeq ($(NO_SDL), 0)
		CXXFLAGS += `sdl2-config --cflags`
		LINK_LIBS += `sdl2-config --libs`
	endif
	ifeq ($(GPIO), 1)
		CXXFLAGS += -DGPIO
		SOURCES+=SPIDevGPIO.cc
		LINK_LIBS += -lpigpio
	endif
endif

OTHER_DEPS=stats_counter_defs.h
OTHER_DEPS+=AbstractPPCDecoder_decoder.h
#OTHER_DEPS+=AbstractPPCDecoder_prototypes.h
OTHER_DEPS+=AbstractPPCDecoder_generators.h
OTHER_DEPS+=PPCInterpreter_prototypes.h
OTHER_DEPS+=PPCInterpreter_mangled.h
OTHER_DEPS+=op_addrs.h

SOURCES+=$(CORE_INTERP_SOURCES)
OBJECTS=$(SOURCES:.cc=.o)
OBJECTS+=$(ASM_SOURCES:.S=.o)
INTERP_OBJECTS=$(CORE_INTERP_SOURCES:.cc=.o)

CXXFLAGS += -DENABLE_COUNTERS=$(COUNTERS)
CXXFLAGS += -DENABLE_TRACING=$(TRACING)
CXXFLAGS += -DENABLE_JIT=$(JIT)

CXXFLAGS += -DFLAT_MEM=$(FLAT_MEM)

all:	sim

sim:	$(OBJECTS)
	$(CXX) $(LINK_FLAGS) $(OBJECTS) $(LINK_LIBS) -o $@

libiss.a:	$(INTERP_OBJECTS) PPCCPUState.o PPCMMU.o stats.o Config.o
	ar r $@ $^

%.o:	%.cc $(OTHER_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o:	%.S $(OTHER_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

PPCInterpreter_auto.cc:
	./tools/mk_decode.py -c PPCInterpreter -i "#include \"PPCInterpreter.h\"" -a $@ tools/PPC.csv

PPCInterpreter_prototypes.h:
	./tools/mk_decode.py -c PPCInterpreter -P $@ tools/PPC.csv
	echo "RET_TYPE op_unk(u32 inst);" >> $@

AbstractPPCDecoder_decoder.h:
	./tools/mk_decode.py -c PPCInterpreter -d $@ tools/PPC.csv

#AbstractPPCDecoder_prototypes.h:
#	./tools/mk_decode.py -c PPCInterpreter -p $@ tools/PPC.csv

AbstractPPCDecoder_generators.h:
	./tools/mk_decode.py -c PPCInterpreter -g $@ tools/PPC.csv

# FIXME: Do better deps analysis here; this is generated from all .cc or .h files.
stats_counter_defs.h:	PPCInterpreter_auto.cc
	./tools/mk_counter_defs . > $@

op_addrs.S: op_list.txt
	./tools/mk_getaddrs $(SYMBOL_PREFIX) < $< > $@

op_addrs.h: op_list.txt
	sed -e 's/\(.*\)/extern "C" u64 get_\1();/;' < $< > $@

op_list.txt: PPCInterpreter_prototypes.h
	sed -e "s/^.*RET_TYPE //; s/(.*//;" < PPCInterpreter_prototypes.h > $@

PPCInterpreter_mangled.h: PPCInterpreter_prototypes.h
	echo '#include "PPCInterpreter.h"' > _tmp.cc
	sed -e "s/^.*RET_TYPE /void * PPCInterpreter::/; s/;/ { return 0; }/;" < PPCInterpreter_prototypes.h >> _tmp.cc
	$(CXX) -c _tmp.cc -o _tmp.o
	$(CMD_SYMBOLS) _tmp.o | grep PPCInterpreter.*op_ | sed -e "s/^.*[^_]\(_*Z\)/\1/;" > _tmp.h
	c++filt < _tmp.h | sed -e 's/^.*PPCInterpreter::/#define CXX_/; s/(.*//;' > _tmp2.h
	paste _tmp2.h _tmp.h > PPCInterpreter_mangled.h
	rm -f _tmp.h _tmp2.h _tmp.cc

clean:
	rm -f sim *~ *.o stats_counter_defs.h AbstractPPCDecoder_prototypes.h PPCInterpreter_prototypes.h AbstractPPCDecoder_decoder.h PPCInterpreter_auto.cc AbstractPPCDecoder_generators.h PPCInterpreter_mangled.h op_addrs.S op_addrs.h op_list.txt libiss.a
