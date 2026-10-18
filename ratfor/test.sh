#!/bin/sh
# Regression test for the CP/M-86 ratfor port.
#
# Pipeline: fib.r -> ratfor.cmd -u -> fib.f77 -> drfcpm_fc -> fib.obj
#                 -> drfcpm_link -> fib.cmd -> emu2 -> output
#
# emu2 does not uppercase the command tail; ratfor.cmd receives filenames
# as given.  DR Fortran-77 requires upper case source, hence ratfor -u.
# drfcpm_fc/drfcpm_link use their own CP/M environment.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=30
fail=0

for binary in ratfor.cmd; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f fib.f77 fib.obj fib.cil fib.cym fib.cmd out.tmp err.tmp' EXIT INT TERM

pass() { printf 'PASS  %s\n' "$1"; }
blame() {
	printf 'FAIL  %s\n' "$1"
	printf '        expected: %s\n' "$2"
	printf '        got:      %s\n' "$3"
	fail=1
}

echo "== ratfor preprocessing =="

# Step 1: preprocess fib.r -> fib.f77 using ratfor.cmd
timeout $TIMEOUT emu2 ratfor.cmd -u -o fib.f77 fib.r > out.tmp 2>&1 < /dev/null
if [ ! -s fib.f77 ]; then
	echo "FAIL  ratfor.cmd produced no output"
	cat out.tmp
	exit 1
fi
pass "ratfor.cmd preprocesses fib.r to fib.f77"

echo "== fortran compilation =="

# Step 2: compile fib.f77 -> fib.obj
drfcpm_fc fib.f77 > out.tmp 2>&1
if [ ! -f fib.obj ]; then
	echo "FAIL  drfcpm_fc produced no fib.obj"
	cat out.tmp
	exit 1
fi
pass "drfcpm_fc compiles fib.f77"

# Step 3: link fib.obj -> fib.cmd
drfcpm_link 'fib.cmd=fib' > out.tmp 2>&1
if [ ! -f fib.cmd ]; then
	echo "FAIL  drfcpm_link produced no fib.cmd"
	cat out.tmp
	exit 1
fi
pass "drfcpm_link links fib.cmd"

echo "== execution =="

# Step 4: run and check output
timeout $TIMEOUT emu2 fib.cmd > out.tmp 2>/dev/null < /dev/null
got=$(tr -d '\r' < out.tmp)

expected="
FIB( 1) =      0
FIB( 2) =      1
FIB( 3) =      1
FIB( 4) =      2
FIB( 5) =      3
FIB( 6) =      5
FIB( 7) =      8
FIB( 8) =     13
FIB( 9) =     21
FIB(10) =     34"

if [ "$got" = "$expected" ]; then
	pass "fib.cmd produces correct fibonacci sequence"
else
	blame "fib.cmd produces correct fibonacci sequence" "$expected" "$got"
fi

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
