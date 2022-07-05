#!/usr/bin/env python3
#
# Turn chonky spreadsheet/CSV of PowerPC instruction descriptions into
# a decoder, and auto-generate implementation functions given a template
# behaviour/action in the CSV.
#
# This outputs a bunch of C++ and headers... mind your eyes, this looks more
# like Perl than Python.
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

import csv
import re
import getopt
import sys
import functools

################################################################################

# Parameter types
param_types = { 'SI':'s16', 'D':'s16', 'UI':'u16', 'LI':'u32', 'LEV':'u8' }
# check BD- s16?
param_default_type = 'int'

needs_accessor = ['RA', 'RA0', 'RB', 'RS', 'RT', 'CR', 'CA']

regval_types = { 'RA':'REG', 'RA0':'REG', 'RB':'REG', 'RS':'REG', 'RT':'REG', 'CR':'uint32_t' }

################################################################################


def fatal(error_string):
    print("ERROR: " + error_string)
    exit(1)


# Read given filename, return list of lines without newlines, and without blank lines:
def read_file_chomp(path):
    with open(path, 'r') as data:
        d = []
        for line in data:
            line = line.rstrip()
            if line != '':
                d.append(line)
        return d


def read_csv(path):
        data = read_file_chomp(path)
        reader = csv.DictReader(data)
        for row in reader:
            yield row


def params_list(regs_list):
    # fugly:
    params = regs_list.split(',')
    # Maintaining order, remove duplicates:
    return functools.reduce(lambda l, x: l+[x] if x not in l else l, params, [])


def gen_extra_regs(has_rc, has_oe, has_aa, has_lk):
    extra_regs = ""
    if has_rc == "1":
        extra_regs += ",Rc"
    if has_oe == "1":
        extra_regs += ",OE"
    if has_aa == "1":
        extra_regs += ",AA"
    if has_lk == "1":
        extra_regs += ",LK"
    return extra_regs


def gen_prototype(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk):
    # Convert to field extraction functions:
    proto_params = []
    extra_regs = gen_extra_regs(has_rc, has_oe, has_aa, has_lk)
    for i in params_list(in_regs + "," + out_regs + extra_regs):
        if i != "":
            if i in param_types:
                pt = param_types[i]
            else:
                pt = param_default_type
            proto_params.append("%s %s" % (pt, i))

    return "RET_TYPE op_%s(%s)" % (name, ', '.join(proto_params))


# Generate a call to a function that generates code to call the given routine
def gen_fn_caller(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk, ends_bb):
    # Convert to field extraction functions:
    call_params = []
    call_types = []
    extra_regs = gen_extra_regs(has_rc, has_oe, has_aa, has_lk)
    for i in params_list(in_regs + "," + out_regs + extra_regs):
        if i != "":
            if i in param_types:
                pt = param_types[i]
            else:
                pt = param_default_type
            call_types.append("%s" % (pt))
            call_params.append("%s_%s(inst)" % (form, i))

    if ends_bb == "":
        rt = "0"
    else:
        rt = ends_bb

    if len(call_params) > 0:
        params = ", " + ', '.join(call_params)
        types = "_" + '_'.join(call_types)
    else:
        params = ""
        types = ""
    return "generate_call%s(codeptr, codelen, get_op_%s()%s); return %s" % (types, name, params, rt)


def gen_fn_decl(obj_class, name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk):
    # Convert to field extraction functions:
    proto_params = []
    extra_regs = gen_extra_regs(has_rc, has_oe, has_aa, has_lk)
    for i in params_list(in_regs + "," + out_regs + extra_regs):
        if i != "":
            if i in param_types:
                pt = param_types[i]
            else:
                pt = param_default_type
            proto_params.append("%s %s" % (pt, i))

    return "RET_TYPE %s::op_%s(%s)" % (obj_class, name, ', '.join(proto_params))


def gen_call(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk):
    # Convert to field extraction functions:
    call_params = []
    extra_regs = gen_extra_regs(has_rc, has_oe, has_aa, has_lk)
    for i in params_list(in_regs + "," + out_regs + extra_regs):
        if i != "":
            call_params.append("%s_%s(inst)" % (form, i))

    return "return static_cast<T*>(this)->op_%s(%s)" % (name, ', '.join(call_params))


def param_needs_accessor(pname):
    if pname in needs_accessor:
        return True
    else:
        return False


def regval_type(p):
    if p in regval_types:
        return regval_types[p]
    else:
        return "int"


def regval_rd_accessor(p):
    if p == 'RA0':
        return "READ_GPR_OZ(%s)" % (p.upper())
    elif p[0] == 'R':
        return "READ_GPR(%s)" % (p.upper())
    else:
        return "READ_%s()" % (p.upper())


def regval_wr_accessor(p, v):
    if p[0] == 'R':
        return "WRITE_GPR(%s, %s)" % (p.upper(), v)
    else:
        return "WRITE_%s(%s)" % (p.upper(), v)


def gen_disasm(name, out, in_list, has_rc, has_oe):
    if has_oe == "1":
        prefix_fmt = "%s"
        prefix_param = ", (OE ? \"o\" : \"\")"
    else:
        prefix_fmt = ""
        prefix_param = ""

    if has_rc == "A":
        prefix_fmt += "."
    elif has_rc == "1":
        prefix_fmt += "%s"
        prefix_param += ", (Rc ? \".\" : \"\")"

    format_string = name + prefix_fmt + "\\t "

    if out != None:
        out_fmt = "%d"
        out_param = ", /* out */ " + out
    else:
        out_fmt = ""
        out_param = ""

    in_fmt = ""
    in_param = ""
    for p in in_list:
        if in_fmt != "":
            in_fmt += ", %d"
        else:
            in_fmt += "%d"
        in_param += ", /* in */ " + p

    if in_fmt != "" and out_fmt != "":
        format_string += out_fmt + ",  " + in_fmt
    else:
        format_string += out_fmt + in_fmt

    return "DISASS(\"%08x   %08x  " + format_string + " \\n\", pc, inst" + prefix_param + out_param + in_param + ");"


def gen_fn_contents(name, in_regs, in_implicit_regs, out_regs, out_implicit_regs,
                    has_rc, has_oe, has_aa, has_lk,
                    action, notes, privilege):
    # Generate function contents
    # Given inputs (including implicit), read regs
    # Given outputs (including implicit), write regs
    # If Rc,SO, set flags (unconditionally if A)

    contents = "\t// Inputs '%s', implicit inputs '%s'\n" % (in_regs, in_implicit_regs)
    in_list = []
    in_explicit_list = []
    if in_regs != "":
        in_explicit_list = in_regs.split(',')
        in_list += in_explicit_list
    if in_implicit_regs != "":
        in_list += in_implicit_regs.split(',')

    out_list = []
    out_explicit_list = []
    if out_regs != "":
        out_explicit_list = out_regs.split(',')
        out_list += out_explicit_list
    if out_implicit_regs != "":
        out_list += out_implicit_regs.split(',')

    for p in in_list:
        if param_needs_accessor(p):
            contents += "\t%s val_%s = %s;\n" % (regval_type(p), p, regval_rd_accessor(p))

    # Declare output vars, unless they've already been declared as inputs (e.g. CA):
    for p in out_list:
        if p not in in_list:
            contents += "\t%s val_%s;\n" % (regval_type(p), p);
    # Declare val_OV, if we're later going to write the OV flags:
    if has_oe == "1":
        contents += "\tint val_OV;\n"

    # Disassembly output
    contents += "\n\t" + gen_disasm(name, out_explicit_list[0] if len(out_explicit_list) != 0 else None, in_explicit_list, has_rc, has_oe)  # ToDo: Support AA,LK
    contents += "\n"

    # Show notes if present
    if notes != "":
        contents += "\n\t/* " + notes + " */\n"

    if privilege == "S":
        contents += "\n\tCHECK_YOUR_PRIVILEGE();\n"
    elif privilege == "H":
        contents += "\n\tCHECK_YOUR_HYP_PRIVILEGE();\n"

    if action != "":
        # The actual function itself:
        contents += "\n\t%s;\n\n" % (re.sub("\s*;\s*", ";\n\t", action))
    else:
        contents += "\n\tUNDEFINED; /* Until implemented */\n\n"

    contents += "\t// Outputs '%s', implicit outputs '%s'\n" % (out_regs, out_implicit_regs)
    for p in out_list:
        if param_needs_accessor(p):
            contents += "\t%s;\n" % (regval_wr_accessor(p, "val_" + p))

    contents += "\n"

    # Misc, flags etc.
    if has_oe == "1":
        contents += "\tif (OE) {\n"
        contents += "\t\tSET_SO_OV(val_OV);\n"
        contents += "\t}\n"

    end_foo = ""
    if has_rc == "1":
        contents += "\tif (Rc) {\n\t"
        end_foo = "\t}\n"
    if has_rc == "1" or has_rc == "A":
        if len(out_list) == 0:
            if action != "":
                # Autogenerated, requires an output reg!
                fatal("%s: No output regs, expected at least 1" % (name))
            else:
                print("%s: No output regs, expected at least 1; continuing" % (name))
                destname = "FIXME"
        else:
            destname = out_list[0]
        # Assume first reg in list is the actual result/RT/etc.
        contents += "\tSET_CR0(val_" + destname + "); // Copies XER.SO\n"
        contents += end_foo

    contents += "\tincPC();\n"
    contents += "\tCOUNT(CTR_INST_%s);\n" % (name.upper())
    contents += "\tINTERP_RETURN;\n"

    return contents


################################################################################

def parse_csv_input(csv_file, cxx_class, verbose = False):
    # Return data:
    #
    # Opcode to list-of-sub-opcodes dict:
    opcodes = dict()
    # Class to list-of-functions-in-class dict:
    fn_info = dict()
    # List of fn prototypes:
    proto_list = []

    for idx, row in enumerate(read_csv(csv_file)):
        name = row['Name']
        form = row['Form']
        classname = row['Class']
        opcode = row['Opcode']
        x_opcode = row['XO']
        subdec = row['Subdec']
        in_regs = row['In'] # itself comma-separated, split
        in_implicit_regs = row['InImpl']
        out_regs = row['Out'] # comma-separated
        out_implicit_regs = row['OutImpl']
        action = row['Action']
        has_rc = row['Rc']
        has_oe = row['SO'] # fixme name
        has_aa = row['AA']
        has_lk = row['LK']
        ends_bb = row['End']
        privilege = row['Priv']
        notes = row['Notes']

        if form != '' and opcode != '':
            if verbose:
                print("%d: %s %s %s %s:%s %s (%s)(%s): \t%s" % (idx, name, classname, form, opcode, x_opcode, subdec, in_regs, out_regs, action))
                print("\t\t %s %s " % (has_rc, has_oe))

            if opcode != '':
                opcode = int(opcode)
            else:
                opcode = 0
            if x_opcode != '':
                x_opcode = int(x_opcode)
            else:
                x_opcode = -1

            if opcode not in opcodes:
                opcodes[opcode] = []

            if classname not in fn_info:
                fn_info[classname] = []

            # The subdec field is blank, 0 or 1.
            # The line is ignored for software/ISS purposes if 1.
            # The line is ignored for hardware/RTL decode purposes if 0.
            if subdec == "1":
                continue

            # Determine fn signature here, give to decoder
            # Add prototypes to classes.
            # Also, generate calling form for decoder to invoke it.
            proto_str = gen_prototype(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk)
            call_str = gen_call(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk)
            fn_decl_str = gen_fn_decl(cxx_class, name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk)
            fn_contents = gen_fn_contents(name,
                                          in_regs, in_implicit_regs,
                                          out_regs, out_implicit_regs,
                                          has_rc, has_oe, has_aa, has_lk,
                                          action, notes, privilege)
            gen_call_str = gen_fn_caller(name, form, in_regs, out_regs, has_rc, has_oe, has_aa, has_lk, ends_bb)

            if x_opcode != -1:
                # Might've seen this opcode before, add it to the list (held in the dict)
                opcodes[opcode].append((name, x_opcode, form, call_str, has_oe, gen_call_str))
            else:
                if len(opcodes[opcode]) != 0:
                    print("WARN:  Row %d (%s) duplicates opcode %d w/o a sub-opcode, ignoring" % (idx, name, opcode))
                opcodes[opcode] = [(name, None, form, call_str, has_oe, gen_call_str)]

            # Contains flag for action presence so that it can generate into an _auto.cc or regular .cc
            fn_info[classname].append((action != "", fn_decl_str, fn_contents));
            proto_list.append(proto_str)

    return (opcodes, fn_info, proto_list)


def gen_decode_switch(opcodes, verbose = False):
    switch_stmt = "switch(opcode) {\n"
    gen_stmt = switch_stmt

    for f in sorted(opcodes):
        xop_list = sorted(opcodes[f], key = lambda x : x[1])
        if verbose:
            print(" Opcode %d:  %d sub-opcode(s):" % (f, len(xop_list)))

        # Singular case might be something like a D-form, or a one-off sub-opcode:
        if len(xop_list) == 1:
            (name, x_opcode, form, call_str, has_oe, gen_call_str) = xop_list[0]
            # Major opcode
            if x_opcode == None:
                if verbose:
                    print("  Major opcode, no xop:\t\t %s  %s-form" % (name, form))
                # Note %%s; this generates a '%s' which is then expanded in two ways
                s = "\tcase 0x%03x: /* %d */\t%%s; \tbreak;  /* %s-form */\n" % (f, f, form)
                switch_stmt += s % (call_str)
                gen_stmt += s % (gen_call_str)
            else:
                # Assumes 10-bit opcode!
                if form != "X":
                    print("WARNING:   Major opcode %d w/ one %s form sub-op %d for %s, FIXME" % (f, form, x_opcode, name))
                if verbose:
                    print("  Major opcode, one xop %d:\t\t %s  %s-form" % (x_opcode, name, form))
                s = "\tcase 0x%03x: /* %d */\tif (%s_XOPC(inst) == %s) { %%s; } \tbreak;  /* %s-form */\n" % (f, f, form, x_opcode, form)
                switch_stmt += s % (call_str)
                gen_stmt += s % (gen_call_str)
        else:
            # Just want the form.  Assumes all forms are the same within an opcode.
            # This is a mess, FIXME!!
            # Deals with X, XO and nothing else.
            (name, x_opcode, form, call_str, has_oe, gen_call_str) = xop_list[0]
            s = "\tcase 0x%03x: /* %d */ {\n\t\tswitch(%s_XOPC(inst)) {\n" % (f, f, form)
            switch_stmt += s
            gen_stmt += s

            for (name, x_opcode, form, call_str, has_oe, gen_call_str) in xop_list:
                if x_opcode == None:
                    fatal("Opcode %d sub-instr %s with no x_opcode" % (f, name))
                else:
                    # 10 bits of XO have been decoded here; X, XL, XFX, XFL, XX1 are OK.  The others are weirder.
                    # Find a better way to do this!
                    # This is a temp hack for XO which is 9 bits instead:
                    if verbose:
                        print("  Minor %d:\t\t\t %s  %s-form" % (x_opcode, name, form))
                    if form == "X" or (form == "XO" and has_oe == 0) or form == "XL" or form == "XFX" or form == "XFL" or form == "XX1":
                        s = "\t\t\tcase 0x%03x: /* %d */\t%%s; \tbreak;  /* %s-form */\n" % (x_opcode, x_opcode, form)
                        switch_stmt += s % (call_str)
                        gen_stmt += s % (gen_call_str)
                    elif form == "XO": # has_oe 1
                        # Hack, just pipe both OE values (bit 21/10) into this fn
                        s = "\t\t\tcase 0x%03x: /* %d */\n" % (x_opcode, x_opcode)
                        s += "\t\t\tcase 0x%03x: /* %d */\t%%s; \tbreak;  /* %s-form */\n" % (x_opcode | 0x200, x_opcode | 0x400, form)
                        switch_stmt += s % (call_str)
                        gen_stmt += s % (gen_call_str)
                    else:
                        fatal("Opcode %d sub-instr %s (op %d) form %s unsupported!" % (f, name, x_opcode, form))
            s = "\t\t\tdefault: break;\n\t\t}\n\t} break;\n"
            switch_stmt += s
            gen_stmt += s

    s = "}\n"
    switch_stmt += s
    gen_stmt += s
    return (switch_stmt, gen_stmt)


def help():
    print("Syntax: this.py -c <classname> [-v] [-i \"#include <foo.h>\"] [] [-d DecoderFile.h] " \
          "[-p ProtosFile.h] [-P ProtosFile.h] [-a AutoGenOutputFile.cc] [-m ManualDir/Path_%s.cc] " \
          "[-g GenFns.h] <defs.csv>")
    print("\t-h\t\t- Help")
    print("\t-v\t\t- Verbose")
    print("\t-c \"Classname\"\t- Class name for function implementations")
    print("\t-i \"string\"\t- Includes added to generated files")
    print("\t-d <file>\t- Output decoder to file")
    print("\t-p <file>\t- Output virtual function prototypes to file")
    print("\t-P <file>\t- Output non-virtual prototypes to file")
    print("\t-a <file>\t- Auto-generate implementation functions to file")
    print("\t-m <path>\t- Output auto-generated manual functions to files at path")
    print("\t-g <file>\t- Auto-generate codegen functions to file")


################################################################################
################################################################################
################################################################################
################################################################################


# Parse command-line args:

verbose = False
classname = ""
decoder_file = ""
virt_protos_file = ""
protos_file = ""
autogen_file = ""
mangen_path = ""
include_string = ""
generator_file = ""

try:
    opts, args = getopt.getopt(sys.argv[1:], "hvc:i:d:p:P:a:m:g:")
except getopt.GetoptError as err:
    help()
    fatal("Invocation error: " + str(err))

for o, a in opts:
    if o == "-h":
        help()
        sys.exit()
    elif o == "-v":
        verbose = True
    elif o == "-c":
        classname = a
    elif o == "-i":
        include_string = a
    elif o == "-d":
        decoder_file = a
    elif o == "-p":
        virt_protos_file = a
    elif o == "-P":
        protos_file = a
    elif o == "-a":
        autogen_file = a
    elif o == "-m":
        mangen_path = a
    elif o == "-g":
        generator_file = a
    else:
        help()
        fatal("Unknown option?")

if classname == "":
    help()
    fatal("Classname -c argument required");

if len(args) > 0:
    input_file = args[0]
else:
    help()
    fatal("Input file required");


################################################################################

# Do the work:

(opcodes, fn_info, proto_list) = parse_csv_input(input_file, classname, verbose)

(switch_stmt, gen_stmt) = gen_decode_switch(opcodes, verbose)

################################################################################

if decoder_file:
    print("Writing decode table to %s" % (decoder_file))
    with open(decoder_file, "w") as output:
        output.write(switch_stmt)

if generator_file:
    print("Writing decode generator table to %s" % (generator_file))
    with open(generator_file, "w") as output:
        output.write(gen_stmt)

if virt_protos_file:
    print("Writing virtual prototypes to %s" % (virt_protos_file))
    with open(virt_protos_file, "w") as output:
        for f in proto_list:
            output.write("virtual %s = 0;\n" % (f))

if protos_file:
    print("Writing non-virtual prototypes to %s" % (protos_file))
    with open(protos_file, "w") as output:
        for f in proto_list:
            output.write("%s;\n" % (f))

if mangen_path or autogen_file:
    auto_file = []
    manu_files = dict()

    for c in sorted(fn_info):
        if c not in manu_files:
            manu_files[c] = []

        fn_list = fn_info[c]
        for p in fn_list:
            (flag, proto, contents) = p
            if flag:        # Has function contents
                auto_file.append((proto, contents));
            else:
                manu_files[c].append((proto, contents));

    if autogen_file:
        print("Writing auto-generated functions to %s" % (autogen_file))

        with open(autogen_file, "w") as output:
            output.write(include_string)
            output.write("\n\n/*****************************************************************************/\n\n")
            for p in auto_file:
                (proto, contents) = p
                output.write("%s\n{\n%s}\n\n" % (proto, contents))

    if mangen_path:
        for c in sorted(manu_files):
            fn_list = manu_files[c]
            if len(fn_list) > 0:
                outname = "%s/%s_%s.cc" % (mangen_path, classname, c)
                print("Writing manual functions (%s) to %s" % (c, outname))
                with open(outname, "w") as output:
                    output.write(include_string)
                    output.write( "\n\n/********************* %s *********************/\n\n" % (c))

                    for p in fn_list:
                        (proto, contents) = p
                        output.write("%s\n{\n%s}\n\n" % (proto, contents))

################################################################################
