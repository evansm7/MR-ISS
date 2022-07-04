#!/bin/bash
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
# A wrapper around MR-ISS sim to help running simple_random test binaries.

TESTS=$*

if [ -z "$TESTS" ] ; then
    echo "this.sh <list of tests.bin/.out>"
    exit 1
fi

TF=`mktemp`
for i in $TESTS; do
    echo "Running $i:"
    ./sim -r ${i}.bin -L 0 2>&1 >/dev/null | grep -A5 "Icount .*[1-9a-f].*" | tail -n 5 | ./tools/regs_to_list.py > $TF
    diff -u ${i}.out $TF
done

rm $TF
