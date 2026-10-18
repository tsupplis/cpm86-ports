#!/bin/sh
# Regression tests for the CP/M-86 grep port.
#
# Both binaries are exercised through emu2. Two target differences matter here:
#
#   - emu2 does not fold the command tail the way the CP/M-86 CCP does, so the
#     patterns below are written in upper case on purpose: that is what grep
#     actually receives on the target.
#   - CP/M-86 has no exit status and the Aztec c86 library sends stderr to the
#     console, so for grep.cmd the status is not checked and diagnostics are
#     looked for in the combined output.
#
# Patterns must not contain a space: emu2 hands arguments over through the DOS
# command tail, which splits them again. Use '.' where a space is meant.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="grep.cmd grep.com"
fail=0

for binary in $TARGETS; do
	if [ ! -f $binary ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f MIXED.TXT OTHER.TXT out.tmp err.tmp' EXIT INT TERM

cat > MIXED.TXT <<'EOF'
Hello World
hello world
HELLO WORLD
100% sure
foo%bar
alpha beta
AAA-bbb
AAA_BBB1C
AAA_BBB1c
AA;bb
UPPERONLY
EOF

cat > OTHER.TXT <<'EOF'
hello again
nothing here
EOF

# CP/M-86 programs terminate through BDOS 0, which carries no exit status.
has_status() {
	case $1 in
	*.cmd) return 1 ;;
	*)     return 0 ;;
	esac
}

run() {
	binary=$1
	shift
	timeout $TIMEOUT emu2 $binary "$@" > out.tmp 2> err.tmp < /dev/null
	rc=$?
	out=$(tr -d '\r' < out.tmp)
	diag=$(cat err.tmp out.tmp | tr -d '\r')
}

pass() {
	printf 'PASS  %-9s %s\n' "$1" "$2"
}

blame() {
	printf 'FAIL  %-9s %s\n' "$1" "$2"
	printf '        argv:     %s\n' "$3"
	printf '        expected: %s\n' "$4"
	printf '        got:      status %s\n' "$rc"
	printf '%s\n' "$5" | sed 's/^/        | /'
	fail=1
}

# check <description> <expected status> <expected stdout> <arg>...
check() {
	desc=$1
	xrc=$2
	xout=$3
	shift 3
	for binary in $TARGETS; do
		run $binary "$@"
		ok=yes
		[ "$out" = "$xout" ] || ok=no
		if has_status $binary; then
			[ "$rc" = "$xrc" ] || ok=no
		fi
		if [ $ok = yes ]; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$*" "status $xrc
$(printf '%s\n' "$xout" | sed 's/^/        | /')" "$out"
		fi
	done
}

# checkdiag <description> <expected status> <message substring> <arg>...
checkdiag() {
	desc=$1
	xrc=$2
	needle=$3
	shift 3
	for binary in $TARGETS; do
		run $binary "$@"
		ok=yes
		printf '%s\n' "$diag" | grep -qF "$needle" || ok=no
		if has_status $binary; then
			[ "$rc" = "$xrc" ] || ok=no
		fi
		if [ $ok = yes ]; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$*" \
				"status $xrc with a message containing \"$needle\"" "$diag"
		fi
	done
}

echo "== literal patterns =="
check 'plain pattern is lower case' 0 'hello world' \
	HELLO MIXED.TXT
check 'no match sets status 1' 1 '' \
	ZZZZ MIXED.TXT
check 'anchored at start' 0 'alpha beta' \
	'^ALPHA' MIXED.TXT
check 'anchored at end' 0 'alpha beta' \
	'BETA$' MIXED.TXT
check 'dot and star' 0 'alpha beta' \
	'AL.*BETA' MIXED.TXT

echo "== case escape =="
check '% raises the whole word' 0 'HELLO WORLD' \
	'%HELLO.%WORLD' MIXED.TXT
check '% toggles back inside a word' 0 'Hello World' \
	'%H%ELLO.%W%ORLD' MIXED.TXT
check 'a non-word character resets the toggle' 0 'AAA-bbb' \
	'%AAA-BBB' MIXED.TXT
check 'digits and _ keep the toggle' 0 'AAA_BBB1C' \
	'%AAA_BBB1C' MIXED.TXT
check 'toggling off inside a word' 0 'AAA_BBB1c' \
	'%AAA_BBB1%C' MIXED.TXT
check 'punctuation resets the toggle' 0 'AA;bb' \
	'%AA;BB' MIXED.TXT
check 'escaped percent is literal' 0 '100% sure' \
	'100\%' MIXED.TXT
check 'escaped percent inside a word' 0 'foo%bar' \
	'FOO\%BAR' MIXED.TXT
check 'upper case character class' 0 'UPPERONLY' \
	'^[%A-%Z]*$' MIXED.TXT

echo "== options =="
check '-I ignores case' 0 'Hello World
hello world
HELLO WORLD' \
	-I HELLO MIXED.TXT
check '-C counts matching lines' 0 '3' \
	-C -I HELLO MIXED.TXT
check '-N prefixes line numbers' 0 '2:hello world' \
	-N HELLO MIXED.TXT
check '-V inverts the match' 0 'Hello World
HELLO WORLD
100% sure
foo%bar
alpha beta
AAA-bbb
AAA_BBB1C
AAA_BBB1c
AA;bb
UPPERONLY' \
	-V HELLO MIXED.TXT
check '-L lists the file name' 0 'MIXED.TXT' \
	-L HELLO MIXED.TXT
check '-S prints nothing' 0 '' \
	-S HELLO MIXED.TXT
check '-E takes the next argument as the pattern' 0 'hello world' \
	-E HELLO MIXED.TXT
check 'lower case option letters work too' 0 '2:hello world' \
	-n HELLO MIXED.TXT

echo "== word matching =="
check '-W matches a whole word' 0 'alpha beta' \
	-W BETA MIXED.TXT
check '-W rejects a partial word' 1 '' \
	-W BET MIXED.TXT

echo "== several files =="
check 'file name prefixes each line' 0 'MIXED.TXT:hello world
OTHER.TXT:hello again' \
	HELLO MIXED.TXT OTHER.TXT
check '-H suppresses the file name' 0 'hello world
hello again' \
	-H HELLO MIXED.TXT OTHER.TXT

echo "== diagnostics =="
checkdiag '-? prints the help' 0 'usage: grep' -?
checkdiag 'no arguments prints the help' 2 'usage: grep'
checkdiag 'unknown flag is rejected' 2 'unknown flag' -Q HELLO MIXED.TXT
checkdiag 'missing file is reported' 2 "can't open" HELLO NOSUCH.TXT

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
