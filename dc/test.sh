#!/bin/sh
# Regression tests for the CP/M-86 dc port.
#
# dc reads from a file or stdin. Under emu2 we pass a script file because
# stdin is /dev/null. The CP/M-86 CCP uppercases the command tail; dc only
# takes a filename argument so that is harmless (filenames are already upper).

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="dc.cmd"
fail=0

for binary in $TARGETS; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f SCRIPT.DC out.tmp err.tmp' EXIT INT TERM

pass() { printf 'PASS  %-9s %s\n' "$1" "$2"; }

blame() {
	printf 'FAIL  %-9s %s\n' "$1" "$2"
	printf '        expected: %s\n' "$3"
	printf '        got:      %s\n' "$4"
	fail=1
}

# run <binary> <script-text>  — result in $out
# Append 'q' so dc exits cleanly instead of falling through to the console.
run() {
	binary=$1; script=$2
	printf '%s\nq\n' "$script" > SCRIPT.DC
	timeout $TIMEOUT emu2 "$binary" SCRIPT.DC > out.tmp 2>/dev/null < /dev/null
	out=$(tr -d '\r' < out.tmp)
}

# check <desc> <expected> <script>
check() {
	desc=$1; xout=$2; script=$3
	for binary in $TARGETS; do
		run "$binary" "$script"
		if [ "$out" = "$xout" ]; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$xout" "$out"
		fi
	done
}

echo "== arithmetic =="
check 'addition'        '7'   '3 4 + p'
check 'subtraction'     '2'   '5 3 - p'
check 'multiplication'  '42'  '6 7 * p'
check 'division'        '3'   '9 3 / p'
check 'modulo'          '1'   '10 3 % p'
check 'negation'        '-5'  '_5 p'
check 'chained ops'     '18'  '2 3 + 4 * 2 - p'

echo "== scale =="
check 'scale division'  '3.33' '2 k 10 3 / p'
check 'sqrt'            '1.41' '2 k 2 v p'

echo "== exponentiation =="
check 'integer power'   '8'   '2 3 ^ p'
check 'power of 10'     '100' '10 2 ^ p'

echo "== stack ops =="
check 'duplicate'       '5
5'  '5 d p p'
check 'clear'           '3'   '1 2 3 c 3 p'
check 'stack depth'     '3'   '7 8 9 z p'

echo "== registers =="
check 'store and load'  '42'  '42 sa la p'
check 'push/pop reg'    '2'   '1 Sa 2 Sa La p'

echo "== base =="
check 'hex output'      'ff'  '16 o 255 p'
check 'hex input'       '255' '16 i FF p'

echo "== strings =="
check 'push string'     'hello' '[hello] P'

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
