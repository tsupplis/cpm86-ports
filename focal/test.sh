#!/bin/sh
# Regression tests for CP/M-86 and DOS Focal ports.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="focal.cmd focal.com"
fail=0

for binary in $TARGETS; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f out.tmp err.tmp prog.foc' EXIT INT TERM

pass() {
	printf 'PASS  %-9s %s\n' "$1" "$2"
}

blame() {
	printf 'FAIL  %-9s %s\n' "$1" "$2"
	printf '        expected: %s\n' "$3"
	printf '        got:      %s\n' "$4"
	fail=1
}

# run_input <binary> <input-string> [args...]
run_input() {
	binary=$1
	input=$2
	shift 2
	printf '%s\nq\n' "$input" | timeout $TIMEOUT emu2 "$binary" "$@" > out.tmp 2> err.tmp
	out=$(tr -d '\r' < out.tmp)
}

# check_eval <desc> <focal-command> <expected-output-substring>
check_eval() {
	desc=$1
	cmd=$2
	expected=$3
	for binary in $TARGETS; do
		run_input "$binary" "$cmd"
		if printf '%s\n' "$out" | grep -qF "$expected"; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$expected" "$out"
		fi
	done
}

# check_run <desc> <script-file> <flag> <expected-output-substring>
check_run() {
	desc=$1
	file=$2
	flag=$3
	expected=$4
	for binary in $TARGETS; do
		run_input "$binary" "" "$file" "$flag"
		if printf '%s\n' "$out" | grep -qF "$expected"; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$expected" "$out"
		fi
	done
}

# check_load_only <desc> <script-file> <expected-line-substring>
check_load_only() {
	desc=$1
	file=$2
	expected=$3
	for binary in $TARGETS; do
		run_input "$binary" "write 10" "$file"
		if printf '%s\n' "$out" | grep -qF "$expected"; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$expected" "$out"
		fi
	done
}

echo "== immediate arithmetic and builtins =="
check_eval 'basic arithmetic' 'type 2+3*4, !' '14.0000'
check_eval 'square root' 'type fsqt(144), !' '12.0000'
check_eval 'sine function' 'type fsin(0), !' '0.0000'
check_eval 'absolute value' 'type fabs(-42), !' '42.0000'
check_eval 'integer truncation' 'type fitr(3.99), !' '3.0000'

echo "== program loops and logic =="
cat > prog.foc <<'EOF'
01.10 set s = 0
01.20 for i = 1, 10; set s = s + i
01.30 type "sum=", s, !
01.40 quit
EOF

check_eval 'inline loop execution' "$(printf '01.10 set s=0\n01.20 for i=1,5; set s=s+i\n01.30 type s, !\ndo 1')" '15.0000'

echo "== command line file loading and execution =="
check_load_only 'load without run stays at prompt' 'queens.foc' '10.10 c main'
check_run 'run with [R]' 'sieve.foc' '[R]' 'Number of primes in 2..1000 is 168'
check_run 'run with [R'  'sieve.foc' '[R'  'Number of primes in 2..1000 is 168'
check_run 'run with [r]' 'sieve.foc' '[r]' 'Number of primes in 2..1000 is 168'
check_run 'run with [r'  'sieve.foc' '[r'  'Number of primes in 2..1000 is 168'
check_run 'run with /R'  'sieve.foc' '/R'  'Number of primes in 2..1000 is 168'
check_run 'run with /r'  'sieve.foc' '/r'  'Number of primes in 2..1000 is 168'
check_run 'queens puzzle [R]' 'queens.foc' '[R]' 'Total 92 answers'

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
