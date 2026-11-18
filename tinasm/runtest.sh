#!/bin/sh
# runtest.sh NAME FORMAT REF -- assemble test/NAME.asm with tinasm.cmd under
# emu2 and compare against the reference file REF (in test/).
#
# CP/M file names are 8.3, so the output is test/NAME.out. The assembler pads
# its output to a multiple of 128 bytes with Ctrl-Z, so only the first
# size-of-REF bytes are compared.
#
# Prints PASS or FAIL and exits 0 or 1.

name=$1
fmt=$2
ref=$3

cd test || exit 1
rm -f "$name.out"
emu2 ../tinasm.cmd -f "$fmt" "$name.asm" -o "$name.out" >"$name.log" 2>&1
size=$(wc -c <"$ref" | tr -d ' ')

if [ ! -f "$name.out" ]; then
    echo "FAIL $name (no output, see test/$name.log)"
    exit 1
fi
if cmp -s -n "$size" "$name.out" "$ref"; then
    echo "PASS $name ($size bytes)"
    rm -f "$name.log"
    exit 0
fi
echo "FAIL $name (differs from $ref, see test/$name.log)"
exit 1
