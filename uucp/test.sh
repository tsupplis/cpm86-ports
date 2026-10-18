#!/bin/sh
# Regression tests for the CP/M-86 uuencode/uudecode port.
#
# emu2 does NOT fold the command tail (unlike the real CCP), so filenames
# and arguments are passed as-is.  We use lowercase throughout.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
fail=0

for binary in uuencode.cmd uuencode.com uudecode.cmd uudecode.com; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f in.bin enc.txt out.bin out.tmp err.tmp testfile' EXIT INT TERM

pass() { printf 'PASS  %s\n' "$1"; }
blame() {
	printf 'FAIL  %s\n' "$1"
	printf '        %s\n' "$2"
	fail=1
}

# 256-byte binary: all byte values 0x00..0xff
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" > in.bin

echo "== uuencode.com | uudecode.cmd =="
emu2 uuencode.com in.bin testfile > enc.txt 2>/dev/null
timeout $TIMEOUT emu2 uudecode.cmd -o out.bin enc.txt > out.tmp 2>/dev/null < /dev/null
if cmp -s in.bin out.bin; then
	pass "uudecode.cmd decodes uuencode.com output correctly"
else
	blame "uudecode.cmd decodes uuencode.com output correctly" "output differs from input"
fi

echo "== uuencode.cmd | uudecode.com =="
timeout $TIMEOUT emu2 uuencode.cmd in.bin testfile > enc.txt 2>/dev/null < /dev/null
emu2 uudecode.com -o out.bin enc.txt 2>/dev/null
if cmp -s in.bin out.bin; then
	pass "uuencode.cmd output decoded correctly by uudecode.com"
else
	blame "uuencode.cmd output decoded correctly by uudecode.com" "output differs from input"
fi

echo "== uuencode.cmd | uudecode.cmd =="
timeout $TIMEOUT emu2 uuencode.cmd in.bin testfile > enc.txt 2>/dev/null < /dev/null
timeout $TIMEOUT emu2 uudecode.cmd -o out.bin enc.txt > out.tmp 2>/dev/null < /dev/null
if cmp -s in.bin out.bin; then
	pass "full CP/M-86 round-trip"
else
	blame "full CP/M-86 round-trip" "output differs from input"
fi

echo "== begin line filename =="
timeout $TIMEOUT emu2 uuencode.cmd in.bin testfile > enc.txt 2>/dev/null < /dev/null
timeout $TIMEOUT emu2 uudecode.cmd enc.txt > out.tmp 2>/dev/null < /dev/null
if cmp -s in.bin testfile 2>/dev/null; then
	pass "uudecode.cmd creates file named in begin line"
else
	blame "uudecode.cmd creates file named in begin line" "testfile missing or wrong"
fi

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
