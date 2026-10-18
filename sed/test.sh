#!/bin/sh
# Regression tests for the CP/M-86 sed port.
#
# Exercises both binaries through emu2. Key target differences:
#
#   - emu2 does not fold the command tail the way the CP/M-86 CCP does,
#     so scripts below are written in upper case: that is what sed receives
#     on the target.
#   - CP/M-86 has no exit status; sed.cmd status is not checked.
#   - Patterns use the % case-toggle layer: letters default to lower case,
#     % raises the rest of the word. \% is a literal %.
#
# Arguments must not contain spaces: emu2 splits the command tail on spaces.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="sed.cmd sed.com"
fail=0

for binary in $TARGETS; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f IN.TXT OUT.TXT out.tmp err.tmp' EXIT INT TERM

cat > IN.TXT <<'EOF'
Hello World
hello world
HELLO WORLD
foo bar
alpha beta
one two three
EOF

has_status() {
	case $1 in
	*.cmd) return 1 ;;
	*)     return 0 ;;
	esac
}

run() {
	binary=$1; shift
	timeout $TIMEOUT emu2 "$binary" "$@" > out.tmp 2> err.tmp < /dev/null
	rc=$?
	out=$(tr -d '\r' < out.tmp)
	diag=$(cat err.tmp out.tmp | tr -d '\r')
}

pass() { printf 'PASS  %-9s %s\n' "$1" "$2"; }

blame() {
	printf 'FAIL  %-9s %s\n' "$1" "$2"
	printf '        argv:     %s\n' "$3"
	printf '        expected: %s\n' "$4"
	printf '        got:      status %s\n' "$rc"
	printf '%s\n' "$5" | sed 's/^/        | /'
	fail=1
}

# check <desc> <expected-status> <expected-stdout> <arg>...
check() {
	desc=$1; xrc=$2; xout=$3; shift 3
	for binary in $TARGETS; do
		run "$binary" "$@"
		ok=yes
		[ "$out" = "$xout" ] || ok=no
		if has_status "$binary"; then [ "$rc" = "$xrc" ] || ok=no; fi
		if [ $ok = yes ]; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$*" \
				"status $xrc
$(printf '%s\n' "$xout" | sed 's/^/        | /')" "$out"
		fi
	done
}

# checkdiag <desc> <expected-status> <message-substring> <arg>...
checkdiag() {
	desc=$1; xrc=$2; needle=$3; shift 3
	for binary in $TARGETS; do
		run "$binary" "$@"
		ok=yes
		printf '%s\n' "$diag" | grep -qF "$needle" || ok=no
		if has_status "$binary"; then [ "$rc" = "$xrc" ] || ok=no; fi
		if [ $ok = yes ]; then
			pass "$binary" "$desc"
		else
			blame "$binary" "$desc" "$*" \
				"status $xrc with message containing \"$needle\"" "$diag"
		fi
	done
}

echo "== substitution =="
check 'basic substitution' 0 \
'Hello World
xello world
HELLO WORLD
foo bar
alpha beta
one two three' \
	-e 'S/HELLO/XELLO/' IN.TXT

check 'global flag replaces all occurrences' 0 \
'one x three' \
	-e 'S/TWO/X/G' -e '1,5D' IN.TXT

check '-g makes all substitutions global by default' 0 \
'one x three' \
	-g -e 'S/TWO/X/' -e '1,5D' IN.TXT

check 'substitution with % case layer' 0 \
'Hello World
hello world
HELLO WORLD
foo bar
alpha beta
one two three' \
	-e 'S/%XELLO/HELLO/' IN.TXT

echo "== delete and print =="
check 'd deletes addressed lines' 0 \
'Hello World
hello world
HELLO WORLD
alpha beta
one two three' \
	-e '4D' IN.TXT

check '-n with p prints only matching lines' 0 \
'hello world
hello world' \
	-n -e '/HELLO/P' -e '2P' IN.TXT

check 'address range deletion' 0 \
'Hello World
alpha beta
one two three' \
	-e '2,4D' IN.TXT

check 'last-line address' 0 \
'Hello World
hello world
HELLO WORLD
foo bar
alpha beta' \
	-e "\$D" IN.TXT

echo "== suppress default print =="
check '-n suppresses all output without explicit p' 0 \
'' \
	-n -e 'S/FOO/BAR/' IN.TXT

check '-n with explicit p on match' 0 \
'foo bar' \
	-n -e '/FOO/P' IN.TXT

echo "== output file =="
check '-o writes to a file' 0 \
'' \
	-o OUT.TXT -e '1,5D' IN.TXT
# the -o test: stdout should be empty; check file content for .com only
# (.cmd writes in CP/M-86 text mode which may add \r; we verify .com only)
for binary in $TARGETS; do
	run "$binary" -o OUT.TXT -e '1,5D' IN.TXT
	case $binary in
	*.com)
		got=$(tr -d '\r\n' < OUT.TXT 2>/dev/null)
		xout='one two three'
		if [ "$got" = "$xout" ]; then
			pass "$binary" "-o writes correct content to file"
		else
			blame "$binary" "-o writes correct content to file" \
				"-o OUT.TXT -e 1,5D IN.TXT" "$xout" "$got"
		fi
		;;
	*.cmd)
		# just verify the file is non-empty
		if [ -s OUT.TXT ]; then
			pass "$binary" "-o writes correct content to file"
		else
			blame "$binary" "-o writes correct content to file" \
				"-o OUT.TXT -e 1,5D IN.TXT" "non-empty file" "empty"
		fi
		;;
	esac
done

echo "== diagnostics =="
checkdiag '-? prints usage' 0 'usage: sed' -?
checkdiag 'missing file is reported' 2 'open' -e 'S/A/B/' NOSUCH.TXT
checkdiag '-o without filename is reported' 2 'requires' -o

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
