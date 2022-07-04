# MR PowerPC Instruction Set Simulator

## Introduction

This is a simple but capable 32-bit PowerPC functional simulator (AKA emulator).  It supports all integer instructions from the PPC604, BAT/MMU translation, cache and TLB maintenance.  It emulates a small set of system devices, and can run Linux.

It does not support floating-point instructions.

At the core is a "big-switch" interpreter; as an exercise, a very simple DBT/JIT generator is also included.  The simple interpreter trades performance for hackability and debuggability:  performance hits about 50 MIPS on my x86-64 laptop, but fast enough for my need.

Out of the box, this project probably doesn't _do_ anything useful to you, but components might be useful.


## History

This project was developed as part of my (working-titled) _Matt-RISC CPU_ hobby project (so you will see various references to "MR"), and was intended to be the "golden reference" for my CPU.

I started hacking on this in about 2017-2018, as an experiment:  I was making a spreadsheet to get to know the PowerPC 604 instruction set, and wondered if I could script out the laborious/annoying parts of building an interpreter.  The majority of the MR-ISS instruction emulation code is auto-generated from the spreadsheet's CSV.  MR-ISS was running Linux in 2018, and was a decent experimental platform for figuring out what I wanted to build in hardware.

In 2020/2021, I worked on the hardware side and got a CPU/SoC running (see OTHER_PROJECTS).  As well as adding the MMU to MR-ISS and emulating the devices for this system, I added state-save so I can use MR-ISS to run fast into complex parts of Linux boot then export the machine state into Verilator for RTL debug.  I also build MR-ISS into the CPU's Verilator harness to co-simulate and check instruction execution results, which is particularly useful for PPC's fiddly logical/carry instructions!

Aside from device emulation and debug features -- there could always be more, and it could always be more complete -- I consider MR-ISS to be complete.


## Excuses

Old story:  I wrote this to be useful, only to me, and quickly rather than planned for release.  This isn't very clean code, and I'm not proud of certain aspects.  :-)  This project was very much treated as a means to an end after a certain point (particularly, adding the MMU and bringing up Linux).  There are various styles that I hate in production code, such as a mix of C/C++ comment styles, some magic numbers/bitfields, and commented-out debug code.  Today, I'm most motivated to get this out there rather than to make it pretty, sorry.  :-)  That said, most of the tricky bits are well-commented.  Bon chance.


## CPU architecture

MR-ISS is userspace-level compatible with the PPC603/604/750, without FP (or SIMD).  FP instructions trap correctly, and can be emulated by the OS (e.g. Linux contains an FPE).  I believe MR-ISS is complete and correct in this regard, but has not undergone industrial-level verification!  It has been compared against G4/e500 hardware with random instruction sequences (using Anton's simple_random) and application execution traces.

Privileged architecture differs:  it does not try to support the implementation-specific HIDx registers.  It supports the MR CPU's IMPDEF SPRs for cache invalidation and debug.  The MMU is fully compatible with the PPC604 (same number of BATs, HTAB, TLB invalidation semantics).  The PPC603-style software-loaded TLB instructions are not supported.


## System architecture

The simulator was built to support the MR CPU/SoC project, so mimics the hardware I've built.  Apart from a very half-arsed 16550 UART, I haven't attempted to emulate any standard/off-the-shelf hardware in order to keep the scope enough (limited to what I'm building in hardware).


## Decoder generation, and The Spreadsheet

All instructions are defined in `tools/PPC.csv`, and the `tools/mk_decode.py` hairball generates C++ `AbstractPPCDecoder_decoder.h` and `PPCInterpreter_auto.cc` from this.

The experiment was to try to automate away the annoying and error-prone boilerplate that's often part of writing interpreters:  the decoder and instruction wrappers are generated.  With some (ab)use of macros, it means most instructions only require one line of code in the "Action" column of the spreadsheet.

For example, the `andc` instruction's Action is simply:  `val_RA = val_RS & ~val_RB`.  Other columns enable CR/XER result flags as required.

Where the corresponding Action is blank for a cell, `mk_decode` generates a skeleton function for the human to fill in.  The remainder of `PPCInterpreter_*.cc` files contain the hand-written implementations of these less-trivial instructions.


## Features

The ISS is generally command-line driven.  The starting state (memory contents, MSR, PC etc.) are configurable, making running scrappy little bits of test code easy to run.  There are various trace and disassembly outputs available.


~~~
Usage:  ./sim <opts>
	-h 		 This help
	-r <path> 	 Set RAM image path (default rom.bin)
	-L <addr> 	 Set RAM image load address
	-b <path> 	 Set blockdev image path (default off)
			 Provide up to 4 times for additional discs
	-v 		 Set verbose
	-d 		 Set disassembly on
	-l <limit> 	 Set instruction count limit
	-p <num> 	 Set periodic state dump
	-s <addr> 	 Set start address
	-m <MSR> 	 Set start MSR value (e.g. 0x40 for high vecs)
	-G <num> 	 Set GPIO inputs (default 0x80000000 for sim)
	-x <path> 	 Save state at exit to file
	-t <trace type> 	 Enable trace:
			syscall 	 Syscall trace
			io 		 IO trace
			branch 		 Branch trace
			mmu 		 MMU trace
			exception 	 Exception trace
			jit 		 JIT trace
~~~

### Devices

*   MR-LCD video controller (via SDL)
*   Serial (MR-uart and a weak 16550-alike): exported to a pty, for use with screen
*   IRQ controller mimicking the Xilinx XPS-intc controller
*   MR-SPI (including backed by real-world GPIO pins on a PiPi, for a kind of real-world "co-simulation" that's useful for driver bringup)
*   GPIO (config switches)
*   SBD (trivial PV block I/O, reinventing the virtio-blk wheel but in 200LoC)
*   MR-SD controller and SD card


### Services

*   Event counters
*   Management interface (read runtime event counter values, graphs)
*   Architectural state save/checkpointing
*   Trivial OS emulation (can run PPC Linux static diet-libc simple test programs)

### JIT

The basic (working) structure of a a JIT is included: a runloop, basic block lookup, and a trivial code generator.  The code generation simply creates a block of call instructions to the interpreter leaf functions corresponding to each instruction.  It doesn't actually generate per-instruction customised code, but the structure is there to do this.  Despite that, it provides decent performance (about 2x the plain interpreter, ~100 MIPS).


## General architecture

(Very wooly!)

A PPCInterpreter object operates on register values contained in a PPCCPUState, and making accesses to a PPCMMU object.  The PPCMMU performs translation and memory accesses on a Bus object.  The Bus matches an accessed physical address to a Device object that serves the corresponding PA range.

The PPCMMU implements a "micro-TLB" which contains translations from both BATs and the HTAB, i.e. the uTLB maps from EA to PA in all cases.  (There are actually multiple uTLBs, for IR/ID on/off cases.)

Since the vast majority of Bus accesses are for actual memory, access to RAM is short-circuited using this One Weird Trick:  when the MMU inserts a translation into the uTLB, it asks the Device providing the PA whether it has a "direct map", i.e. whether the physical page is fully present in the host address space.  (A UART isn't: an access is programmatically dealt with.  RAM is: it's an mmap.)  A direct device address stores the _host_ VA in the uTLB; when the uTLB entry is used, the address is dereferenced directly instead of going into Bus.  This is nice and quick.

Interrupts and timer events are flagged to the runloop in main, which raises an exception (on PPCCPUState).  Other instruction-based and memory access traps do the same, asking PPCCPUState to take an exception (change PC/SRR0/SRR1) and then continuing on at the respective vector.


# Building

Just `make` it (TM).  MR-ISS builds on Linux and Darwin, but requires x86-64 if you want to try the JIT (you don't).  Needs python, bash, and SDL2 (if you want video output).

The Makefile takes trivial flags to configure the build, e.g.:

> make FLAG=1

Unless otherwise specified, flags are booleans (0 false, 1 true).

| Option | Meaning | Default |
| ------ | ------- | ------- |
| FLAT_MEM 	| Don't build MMU/address translation, treat memory as flat. | Off, build MMU |
| DEBUG 		| Turn off compiler optimisation, add debug symbols etc. | Off, use -O3 |
| NO_DIVIDE	| Trap on divide instructions. | Off, support divide instructions |
| HOSTED		| Catch syscalls, provide a trivial OS environment for basic I/O | Off, bare-metal, syscalls -> 0xc00 |
| COUNTERS	| Include event counters, at a small performance cost | On, include counters |
| TRACING 	| Include runtime trace messages (see '-t' switch), at a small performance cost | On, include tracing |
| JIT			| Experimental trivial JIT support (on x86-64 hosts) | Off |
| PLATFORM	| Platform selection | Platform 3 |
| ALIGNMENT	| Memory access alignment strictness: 0 = any unaligned ops trap; 1 = permit unaligned within a 64b boundary; 2 = permit any unaligned ops. | 1 |
| NO_SDL		| Disable SDL for LCDC output (i.e. disables LCD output, though device is still emulated) | Off, include SDL |
| GPIO			| Enable SPIDevGPIO, for real-world SPI devices, e.g. on Raspberry Pi hosts | Off, no GPIO |
| UPDATE_RC	| HTAB referenced/changed flag update: Linux works w/o update of PTE R/C bits, so disabling this update will improve sim performance a little (despite being architecturally incorrect :P).  Plus, interesting for experimentation. | On, update R/C flags |


# Demo

A simple bare-metal demo program is supplied in `test`, which draws to the LCD framebuffer and outputs via the UART.  It needs a PowerPC toolchain (e.g. `powerpc-linux-gnu`) to build.

~~~
cd MR-ISS
# Build MR-ISS
make
# Build the demo
cd test
make
# Run the demo (test.bin loaded at/executed from address 0):
../sim -r test.bin -L 0
# Hilarity ensues 
~~~

While this is running, try `telnet localhost 8888`; this is the management interface.  There is `help`.  You can do things like `stats` to read the realtime event counters, `cpu` for CPU state, or turn on disassembly logging using `setlog LOG_FLAG_DISASS 1`.  It's really meant for scripts, so isn't super-friendly.


# To-do

*   Make pretty
*   Command-line switch for enabling OS emulation/trapping (rather than the HOSTED build)
*   Refactor to make adding a non-PPCInterpreter CPU easier
*   Add the MR sim as a CPU (somehow interface to CPU state which is used everywhere)
*   Double-check privilege checks w.r.t. instr decode correct mtmsr/mfmsr, rfi, etc.



# Example output from Linux:

(Sneaky, because this kernel is far from anything mainline with various custom drivers; they will appear in due course, but this at least shows the instruction emulation and MMU is robust enough.)

~~~
SerialPTY:  FD 4 connected to /dev/ttys009
Bus devices:
	00:	00000000 - 20000000	0x10135cf30
	01:	80000000 - 80000200	0x10135ceb0
	02:	80010000 - 80010200	0x10135d170
	03:	80020000 - 80020020	0x10135d1d8

zImage starting: loaded at 0x08000000 (sp: 0x082a6fa0)
Allocating 0x584dc8 bytes for kernel...
Decompressing (0x00000000 <- 0x0800c000:0x082a57f6)...
Done! Decompressed 0x567f1c bytes

Linux/PowerPC load: console=ttyS0 earlyprintk debug root=/dev/sbd1
Finalizing device tree... flat tree at 0x82a7900
bootconsole [udbg0] enabled
Total memory = 512MB; using 1024kB for hash table (at cff00000)
Linux version 4.13.0-00016-gd3494053c493-dirty (matt@bin) (gcc version 7.3.0 (Ubuntu 7.3.0-16ubuntu3)) #114 Sun Sep 16 14:51:57 BST 2018
Using MR machine description
Found legacy serial port 0 for /soc/serial@80000000
  mem=80000000, taddr=80000000, irq=0, clk=1067212800, speed=115200
-----------------------------------------------------
Hash_size         = 0x100000
phys_mem_size     = 0x20000000
dcache_bsize      = 0x20
icache_bsize      = 0x20
cpu_features      = 0x0000000000200040
  possible        = 0x0000000005a6fd77
  always          = 0x0000000000000000
cpu_user_features = 0x84000000 0x00000000
mmu_features      = 0x00000001
Hash              = 0xcff00000
Hash_mask         = 0x3fff
-----------------------------------------------------
mr_setup_arch()
MR Platform
Top of RAM: 0x20000000, Total RAM: 0x20000000
Memory hole size: 0MB
Zone ranges:
  DMA      [mem 0x0000000000000000-0x000000001fffffff]
  Normal   empty
Movable zone start for each node
Early memory node ranges
  node   0: [mem 0x0000000000000000-0x000000001fffffff]
Initmem setup node 0 [mem 0x0000000000000000-0x000000001fffffff]
On node 0 totalpages: 131072
free_area_init_node: node 0, pgdat c0554f10, node_mem_map dfb78000
  DMA zone: 1152 pages used for memmap
  DMA zone: 0 pages reserved
  DMA zone: 131072 pages, LIFO batch:31
pcpu-alloc: s0 r0 d32768 u32768 alloc=1*32768
pcpu-alloc: [0] 0 
Built 1 zonelists in Zone order, mobility grouping on.  Total pages: 129920
Kernel command line: console=ttyS0 earlyprintk debug root=/dev/sbd1
PID hash table entries: 2048 (order: 1, 8192 bytes)
Dentry cache hash table entries: 65536 (order: 6, 262144 bytes)
Inode-cache hash table entries: 32768 (order: 5, 131072 bytes)
Memory: 512540K/524288K available (4216K kernel code, 212K rwdata, 928K rodata, 168K init, 115K bss, 11748K reserved, 0K cma-reserved)
Kernel virtual memory layout:
  * 0xfffdf000..0xfffff000  : fixmap
  * 0xfdfff000..0xfe000000  : early ioremap
  * 0xe1000000..0xfdfff000  : vmalloc & ioremap
SLUB: HWalign=32, Order=0-3, MinObjects=0, CPUs=1, Nodes=1
NR_IRQS: 512, nr_irqs: 512, preallocated irqs: 16
irq-xilinx: /soc/interrupt-controller@80010000: num_irq=2, edge=0x3
time_init: decrementer frequency = 25.000000 MHz
time_init: processor frequency   = 100.000000 MHz
clocksource: timebase: mask: 0xffffffffffffffff max_cycles: 0x5c40939b5, max_idle_ns: 440795202646 ns
clocksource: timebase mult[28000000] shift[24] registered
clockevent: decrementer mult[6666666] shift[32] cpu[0]
Console: colour dummy device 80x25
pid_max: default: 32768 minimum: 301
Mount-cache hash table entries: 1024 (order: 0, 4096 bytes)
Mountpoint-cache hash table entries: 1024 (order: 0, 4096 bytes)
devtmpfs: initialized
clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 19112604462750000 ns
futex hash table entries: 256 (order: -1, 3072 bytes)
random: get_random_u32 called from bucket_table_alloc+0xa0/0x208 with crng_init=0
NET: Registered protocol family 16
             
PCI: Probing PCI hardware
random: fast init done
vgaarb: loaded
clocksource: Switched to clocksource timebase
NET: Registered protocol family 2
TCP established hash table entries: 4096 (order: 2, 16384 bytes)
TCP bind hash table entries: 4096 (order: 2, 16384 bytes)
TCP: Hash tables configured (established 4096 bind 4096)
UDP hash table entries: 256 (order: 0, 4096 bytes)
UDP-Lite hash table entries: 256 (order: 0, 4096 bytes)
NET: Registered protocol family 1
PCI: CLS 0 bytes, default 32
workingset: timestamp_bits=14 max_order=17 bucket_order=3
io scheduler noop registered
io scheduler deadline registered
io scheduler cfq registered (default)
io scheduler mq-deadline registered
io scheduler kyber registered
Serial: 8250/16550 driver, 4 ports, IRQ sharing enabled
console [ttyS0] disabled
serial8250.0: ttyS0 at MMIO 0x80000000 (irq = 16, base_baud = 66700800) is a 16450
console [ttyS0] enabled
console [ttyS0] enabled
bootconsole [udbg0] disabled
bootconsole [udbg0] disabled
console [ttyS0] disabled
console [ttyS0] enabled
brd: module loaded
loop: module loaded
sbd 80020000.sbd: SimpleBlockDevice driver initialised, magic feedf00d, blocks 2097152 (1073741824 bytes, sector size 512)
 sbd: sbd1
SBD device driver, major=254
Initializing XFRM netlink socket
NET: Registered protocol family 10
Segment Routing with IPv6
sit: IPv6, IPv4 and MPLS over IPv4 tunneling driver
NET: Registered protocol family 17
hctosys: unable to open rtc device (rtc0)
EXT4-fs (sbd1): mounting ext3 file system using the ext4 subsystem
EXT4-fs (sbd1): mounted filesystem with ordered data mode. Opts: (null)
VFS: Mounted root (ext3 filesystem) readonly on device 254:1.
devtmpfs: mounted
Freeing unused kernel memory: 168K
This architecture does not have kernel memory protection.
Mount failed for selinuxfs on /sys/fs/selinux:  No such file or directory
INIT: version 2.88 booting
[info] Using makefile-style concurrent boot in runlevel S.
calling: info
[....] Starting the hotplug events dispatcher: udevdsystemd-udevd[971]: starting version 215
. ok 
[....] Synthesizing the initial hotplug events...calling: trigger
done.
[....] Waiting for /dev to be fully populated...calling: settle
Shutting down.

Global counters:
	                       CTR_EXCEPTION_DEC:	457
	                       CTR_EXCEPTION_IRQ:	73
	                       CTR_EXCEPTION_MEM:	26609
	                      CTR_EXCEPTION_PROG:	43016
	                            CTR_INST_ADD:	18471111
	                           CTR_INST_ADDC:	286169
	                           CTR_INST_ADDE:	183628
	                           CTR_INST_ADDI:	47334757
	                          CTR_INST_ADDIC:	810728
	                       CTR_INST_ADDIC_RC:	422933
	                          CTR_INST_ADDIS:	7990310
	                          CTR_INST_ADDME:	27354
	                          CTR_INST_ADDZE:	265177
	                            CTR_INST_AND:	5296511
	                           CTR_INST_ANDC:	1845792
	                       CTR_INST_ANDIS_RC:	958217
	                        CTR_INST_ANDI_RC:	11949255
	                              CTR_INST_B:	14814530
	                             CTR_INST_BC:	79554832
	                          CTR_INST_BCCTR:	1719428
	                           CTR_INST_BCLR:	8539214
	                            CTR_INST_CMP:	4543550
	                           CTR_INST_CMPI:	21463385
	                           CTR_INST_CMPL:	13857087
	                          CTR_INST_CMPLI:	7132227
	                         CTR_INST_CNTLZW:	654401
	                          CTR_INST_CRAND:	56499
	                         CTR_INST_CRANDC:	0
	                          CTR_INST_CREQV:	0
	                         CTR_INST_CRNAND:	0
	                          CTR_INST_CRNOR:	3016
	                           CTR_INST_CROR:	1788
	                          CTR_INST_CRORC:	0
	                          CTR_INST_CRXOR:	31581
	                           CTR_INST_DCBA:	0
	                           CTR_INST_DCBF:	178268
	                           CTR_INST_DCBI:	0
	                          CTR_INST_DCBST:	1031372
	                           CTR_INST_DCBT:	888182
	                         CTR_INST_DCBTST:	137491
	                           CTR_INST_DCBZ:	2677616
	                           CTR_INST_DIVW:	627
	                          CTR_INST_DIVWU:	168103
	                          CTR_INST_EIEIO:	476
	                            CTR_INST_EQV:	53453
	                       CTR_INST_EXECUTED:	531000001
	                           CTR_INST_EXIT:	0
	                          CTR_INST_EXTSB:	148369
	                          CTR_INST_EXTSH:	87981
	                           CTR_INST_ICBI:	1209640
	                           CTR_INST_ICBT:	0
	                          CTR_INST_ISYNC:	461303
	                            CTR_INST_LBZ:	11454709
	                           CTR_INST_LBZU:	5432745
	                          CTR_INST_LBZUX:	50151
	                           CTR_INST_LBZX:	3053197
	                            CTR_INST_LHA:	3485
	                           CTR_INST_LHAU:	0
	                          CTR_INST_LHAUX:	0
	                           CTR_INST_LHAX:	13800
	                          CTR_INST_LHBRX:	11269
	                            CTR_INST_LHZ:	3600480
	                           CTR_INST_LHZU:	29968
	                          CTR_INST_LHZUX:	16431
	                           CTR_INST_LHZX:	209010
	                            CTR_INST_LMW:	3097
	                           CTR_INST_LSWI:	0
	                           CTR_INST_LSWX:	0
	                          CTR_INST_LWARX:	1887243
	                          CTR_INST_LWBRX:	47011
	                            CTR_INST_LWZ:	74970213
	                           CTR_INST_LWZU:	4837212
	                          CTR_INST_LWZUX:	681
	                           CTR_INST_LWZX:	2525283
	                           CTR_INST_MCRF:	80065
	                          CTR_INST_MCRXR:	0
	                           CTR_INST_MFCR:	788032
	                          CTR_INST_MFMSR:	782384
	                          CTR_INST_MFSPR:	6619394
	                           CTR_INST_MFSR:	0
	                         CTR_INST_MFSRIN:	13790
	                          CTR_INST_MTCRF:	1212687
	                          CTR_INST_MTMSR:	1451288
	                          CTR_INST_MTSPR:	8477001
	                           CTR_INST_MTSR:	0
	                         CTR_INST_MTSRIN:	15592
	                          CTR_INST_MULHW:	2397
	                         CTR_INST_MULHWU:	80924
	                          CTR_INST_MULLI:	923678
	                          CTR_INST_MULLW:	404281
	                           CTR_INST_NAND:	990
	                            CTR_INST_NEG:	375148
	                            CTR_INST_NOR:	1502264
	                             CTR_INST_OR:	29646879
	                            CTR_INST_ORC:	159050
	                            CTR_INST_ORI:	3064145
	                           CTR_INST_ORIS:	266447
	                            CTR_INST_RFI:	196683
	                         CTR_INST_RLWIMI:	1084217
	                         CTR_INST_RLWINM:	22815887
	                          CTR_INST_RLWNM:	26653
	                             CTR_INST_SC:	35080
	                            CTR_INST_SLW:	4281016
	                           CTR_INST_SRAW:	36245
	                          CTR_INST_SRAWI:	518276
	                            CTR_INST_SRW:	4398061
	                            CTR_INST_STB:	3356344
	                           CTR_INST_STBU:	564554
	                          CTR_INST_STBUX:	0
	                           CTR_INST_STBX:	594834
	                            CTR_INST_STH:	352268
	                         CTR_INST_STHBRX:	0
	                           CTR_INST_STHU:	1639777
	                          CTR_INST_STHUX:	0
	                           CTR_INST_STHX:	54866
	                           CTR_INST_STMW:	6346
	                  CTR_INST_STRING_LD_ODD:	0
	            CTR_INST_STRING_LD_UNALIGNED:	0
	                  CTR_INST_STRING_ST_ODD:	0
	            CTR_INST_STRING_ST_UNALIGNED:	0
	                          CTR_INST_STSWI:	0
	                          CTR_INST_STSWX:	0
	                            CTR_INST_STW:	48935064
	                         CTR_INST_STWBRX:	10841
	                          CTR_INST_STWCX:	1965224
	                           CTR_INST_STWU:	8813735
	                          CTR_INST_STWUX:	1827
	                           CTR_INST_STWX:	1117678
	                           CTR_INST_SUBF:	7765522
	                          CTR_INST_SUBFC:	158712
	                          CTR_INST_SUBFE:	357097
	                         CTR_INST_SUBFIC:	303852
	                         CTR_INST_SUBFME:	0
	                         CTR_INST_SUBFZE:	35
	                           CTR_INST_SYNC:	442752
	                          CTR_INST_TLBIA:	0
	                          CTR_INST_TLBIE:	68957
	                         CTR_INST_TLBIEL:	0
	                        CTR_INST_TLBSYNC:	0
	                             CTR_INST_TW:	0
	                            CTR_INST_TWI:	309661
	                            CTR_INST_UNK:	43016
	                            CTR_INST_XOR:	1172079
	                           CTR_INST_XORI:	207186
	                          CTR_INST_XORIS:	78141
	                        CTR_MEM_HASH_HIT:	761140
	                     CTR_MEM_HASH_LOOKUP:	784534
	                             CTR_MEM_R16:	3895712
	                   CTR_MEM_R16_UNALIGNED:	5
	                             CTR_MEM_R32:	84414870
	                   CTR_MEM_R32_UNALIGNED:	27442
	                              CTR_MEM_R8:	22410082
	                              CTR_MEM_RI:	531000001
	                        CTR_MEM_UTLB_HIT:	730490021
	                       CTR_MEM_UTLB_MISS:	4989391
	                             CTR_MEM_W16:	2046911
	                   CTR_MEM_W16_UNALIGNED:	13
	                             CTR_MEM_W32:	82203495
	                   CTR_MEM_W32_UNALIGNED:	11227
	                              CTR_MEM_W8:	4515732
~~~

# Copyright and Licence

MR-ISS is MIT-licenced.  I hope this code is helpful to you; if you use any of it, please give me a credit.

Copyright 2016-2022 Matt Evans

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

