#!/usr/bin/env python3
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

import sys
import re

regvals = dict()

inlines = ""
for line in sys.stdin:
    inlines += line

l = inlines.split()

for i in range(len(l)//2):
    reg = l[i*2]
    r = re.match(r"r([0-9]*)", reg)
    if r:
        reg = int(r.group(1))
    val = l[i*2+1].upper()
    if len(val) < 16:
        val = "00000000" + val
    regvals[reg] = val

# Format output the same as simple_random:

for i in range(31):
    r = 'GPR%d' % i
    print("%s %s" % (r, regvals[i]))
print("GPR31 ")
print("CR %s" % (regvals['CR']))
print("LR %s" % (regvals['LR']))
print("CTR %s" % (regvals['CTR']))
print("XER %s" % (regvals['XER']))
print("")
#print("")
