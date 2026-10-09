#!/bin/sh
# Regression tests for the CP/M-86 uuencode/uudecode port.
#
# Strategy: encode a known binary with uuencode.com (DOS, reliable),
# then decode it with uudecode.cmd (CP/M-86 target under test), and
# vice versa.  Also round-trip both .cmd binaries together.
#
# The CCP uppercases the command tail; filenames are already upper case.
# emu2 does not fold the tail, so we write arguments in upper case.

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

trap 'rm -f IN.BIN ENC.TXT OUT.BIN out.tmp err.tmp' EXIT INT TERM

pass() { printf 'PASS  %s\n' "$1"; }
blame() {
	printf 'FAIL  %s\n' "$1"
	printf '        %s\n' "$2"
	fail=1
}

# create a known binary test file: 256 bytes, values 0x00..0xff
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" > IN.BIN

echo "== uuencode.com | uudecode.cmd =="
# encode with the DOS binary (known good)
emu2 uuencode.com IN.BIN TESTFILE > ENC.TXT 2>/dev/null
# decode with the CP/M-86 binary
timeout $TIMEOUT emu2 uudecode.cmd -o OUT.BIN ENC.TXT > out.tmp 2>/dev/null < /dev/null
if cmp -s IN.BIN OUT.BIN; then
	pass "uudecode.cmd round-trips 256-byte binary"
else
	blame "uudecode.cmd round-trips 256-byte binary" "output differs from input"
fi

echo "== uuencode.cmd | uudecode.com =="
# encode with the CP/M-86 binary
timeout $TIMEOUT emu2 uuencode.cmd IN.BIN TESTFILE > ENC.TXT 2>/dev/null < /dev/null
# decode with the DOS binary (known good)
emu2 uudecode.com -o OUT.BIN ENC.TXT 2>/dev/null
if cmp -s IN.BIN OUT.BIN; then
	pass "uuencode.cmd round-trips 256-byte binary"
else
	blame "uuencode.cmd round-trips 256-byte binary" "output differs from input"
fi

echo "== uuencode.cmd | uudecode.cmd =="
timeout $TIMEOUT emu2 uuencode.cmd IN.BIN TESTFILE > ENC.TXT 2>/dev/null < /dev/null
timeout $TIMEOUT emu2 uudecode.cmd -o OUT.BIN ENC.TXT > out.tmp 2>/dev/null < /dev/null
if cmp -s IN.BIN OUT.BIN; then
	pass "uuencode.cmd + uudecode.cmd full CP/M-86 round-trip"
else
	blame "uuencode.cmd + uudecode.cmd full CP/M-86 round-trip" "output differs from input"
fi

echo "== begin line filename =="
# uudecode.cmd should create the file named in the begin line
rm -f TESTFILE
timeout $TIMEOUT emu2 uuencode.cmd IN.BIN TESTFILE > ENC.TXT 2>/dev/null < /dev/null
timeout $TIMEOUT emu2 uudecode.cmd ENC.TXT > out.tmp 2>/dev/null < /dev/null
if cmp -s IN.BIN TESTFILE 2>/dev/null; then
	pass "uudecode.cmd uses filename from begin line"
else
	blame "uudecode.cmd uses filename from begin line" "TESTFILE missing or wrong"
fi
rm -f TESTFILE

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
