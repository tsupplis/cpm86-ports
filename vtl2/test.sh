#!/bin/sh
# Regression tests for the VTL-2 port: runs the examples under both the
# CP/M-80 and CP/M-86 binaries and checks the output.
#
# Input comes from a script file rather than a pipe on purpose: VTL-2 polls
# console status after each message and would eat piped characters.

cd "$(dirname "$0")" || exit 1

# The emulators can emit bytes that are not valid UTF-8, which upsets tr.
LC_ALL=C
export LC_ALL

TIMEOUT=2
fail=0

emulator_for() {
	case $1 in
	com) echo "tnylpo vtl2.com" ;;
	cmd) echo "emu2 vtl2.cmd" ;;
	esac
}

report() {
	if [ "$2" = yes ]; then
		printf 'PASS  %-12s %s\n' "$3" "$4"
	else
		printf 'FAIL  %-12s %s\n' "$3" "$4"
		printf '%s\n' "$1" | sed 's/^/        | /'
		fail=1
	fi
}

# want <script> <pattern>...   every pattern must appear in the output
# ANSWERS, if set, is typed at the console prompts.
want() {
	script=$1
	shift
	for target in com cmd; do
		if [ -n "$ANSWERS" ]; then
			printf '%b' "$ANSWERS" > answers.tmp
			input=answers.tmp
		else
			input=/dev/null
		fi
		out=$(timeout $TIMEOUT $(emulator_for $target) "$script" \
			< $input 2>&1 | tr -d '\r')
		ok=yes
		for pattern in "$@"; do
			printf '%s\n' "$out" | grep -qF "$pattern" || ok=no
		done
		report "$out" "$ok" "$script" "$target"
	done
	ANSWERS=
}

# reject <script> <pattern>    the pattern must not appear
reject() {
	for target in com cmd; do
		out=$(timeout $TIMEOUT $(emulator_for $target) "$1" \
			< /dev/null 2>&1 | tr -d '\r')
		ok=yes
		printf '%s\n' "$out" | grep -qF "$2" && ok=no
		report "$out" "$ok" "$1" "$target"
	done
}

for binary in vtl2.com vtl2.cmd; do
	if [ ! -f $binary ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

echo "== examples =="
want ex0.vtl 'The average is 6'
ANSWERS='5\r\n6\r\n7\r\n' want ex1.vtl 'Enter three values' 'The average is 6'
want ex2.vtl '0 1 1 2 3 5 8 13 21 34 55 89'
ANSWERS='10\r\n' want ex3.vtl 'How many terms' '0 1 1 2 3 5 8 13 21 34'

# ex4 has no #=1, so it must load the program and print nothing.
echo "== load only =="
reject ex4.vtl 'The average is'

# 99, two rubouts, 5 leaves B=5; the leading rubouts must be ignored.
echo "== rubout =="
trap 'rm -f rubout.tmp answers.tmp' EXIT INT TERM
printf '\177\17710 B=99\177\1775\r\n20 ?=B\r\n30 ?=""\r\n#=1\r\n>=\r\n' > rubout.tmp
want rubout.tmp '5'
printf '\010\01010 B=99\010\0105\r\n20 ?=B\r\n30 ?=""\r\n#=1\r\n>=\r\n' > rubout.tmp
want rubout.tmp '5'

# No argument means interactive, so the OK prompt must still appear.
echo "== prompt =="
for target in com cmd; do
	out=$(timeout $TIMEOUT $(emulator_for $target) < /dev/null 2>&1 |
		head -c 40 | tr -d '\r')
	ok=yes
	printf '%s\n' "$out" | grep -qF 'OK' || ok=no
	report "$out" "$ok" '(no file)' "$target"
done

# A loaded script suppresses that prompt before its own output.
echo "== no prompt before script output =="
for target in com cmd; do
	out=$(timeout $TIMEOUT $(emulator_for $target) ex0.vtl < /dev/null 2>&1 |
		tr -d '\r')
	ok=yes
	[ "$(printf '%s\n' "$out" | sed -n '1p')" = 'VTL2 Interpreter 1.0' ] || ok=no
	[ "$(printf '%s\n' "$out" | sed -n '2p')" = 'The average is 6' ] || ok=no
	report "$out" "$ok" 'ex0.vtl' "$target"
done

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
