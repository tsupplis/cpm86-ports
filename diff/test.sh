#!/bin/sh
# Regression tests for the CP/M-86 diff port.
#
# Exercises both binaries through emu2. Key target differences:
#
#   - emu2 does not fold the command tail the way the CP/M-86 CCP does,
#     so file names and option arguments below are written in upper case.
#   - CP/M-86 has no exit status; diff.cmd status is not checked.
#   - Two explicit file arguments are always required (no stdin, no dirs).

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="diff.cmd diff.com"
fail=0

for binary in $TARGETS; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f OLD.TXT NEW.TXT BIN1.TXT BIN2.TXT OUT.TXT out.tmp err.tmp' \
	EXIT INT TERM

# OLD.TXT — base file
cat > OLD.TXT <<'EOF'
alpha
beta
gamma
delta
epsilon
EOF

# NEW.TXT — same as OLD except: line 2 changed, line 4 deleted, line appended
cat > NEW.TXT <<'EOF'
alpha
BETA
gamma
epsilon
zeta
EOF

# Binary files (contain a high byte)
printf 'abc\200def\n' > BIN1.TXT
printf 'abc\201def\n' > BIN2.TXT

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

echo "== identical files =="
check 'identical files exit 0 with no output' 0 \
	'' \
	OLD.TXT OLD.TXT

echo "== normal diff =="
check 'full diff OLD vs NEW' 1 \
'2c2
< beta
---
> BETA
4d3
< delta
5a5
> zeta' \
	OLD.TXT NEW.TXT

# Isolate the delete: compare OLD against a file with line 4 removed
cat > OUT.TXT <<'EOF'
alpha
beta
gamma
epsilon
EOF
check 'deleted line produces d record' 1 \
'4d3
< delta' \
	OLD.TXT OUT.TXT

# Isolate the append: compare OLD against a file with a line appended
cat > OUT.TXT <<'EOF'
alpha
beta
gamma
delta
epsilon
zeta
EOF
check 'appended line produces a record' 1 \
'5a6
> zeta' \
	OLD.TXT OUT.TXT

echo "== ignore options =="
# -i: beta vs BETA should now match
cat > OUT.TXT <<'EOF'
alpha
BETA
gamma
delta
epsilon
EOF
check '-i ignores case: no differences' 0 \
	'' \
	-I OLD.TXT OUT.TXT

# -b: trailing-blank difference should be ignored
cat > OUT.TXT <<'EOF'
alpha
beta   
gamma
delta
epsilon
EOF
check '-b ignores trailing blanks: no differences' 0 \
	'' \
	-B OLD.TXT OUT.TXT

echo "== context diff =="
check '-c produces context header and markers' 1 \
'*** OLD.TXT
--- NEW.TXT
***************
*** 1,5 ****
  alpha
! beta
  gamma
- delta
  epsilon
--- 1,5 ----
  alpha
! BETA
  gamma
  epsilon
+ zeta' \
	-C OLD.TXT NEW.TXT

echo "== ed script =="
check '-e produces ed script' 1 \
'5a
zeta
.
4d
2c
BETA
.' \
	-E OLD.TXT NEW.TXT

echo "== output file =="
for binary in $TARGETS; do
	run "$binary" -O OUT.TXT OLD.TXT NEW.TXT
	got=$(tr -d '\r' < OUT.TXT 2>/dev/null)
	# stdout (out.tmp) should be empty; content should be in OUT.TXT
	stdout_content=$(tr -d '\r' < out.tmp)
	if [ -z "$stdout_content" ] && printf '%s\n' "$got" | grep -q '^2c2'; then
		pass "$binary" "-o writes diff to file, nothing to screen"
	else
		blame "$binary" "-o writes diff to file, nothing to screen" \
			"-O OUT.TXT OLD.TXT NEW.TXT" \
			"empty stdout, file contains change record" \
			"stdout='$stdout_content' file='$got'"
	fi
done

echo "== binary files =="
checkdiag 'binary files are detected' 1 'Binary files' BIN1.TXT BIN2.TXT

echo "== diagnostics =="
checkdiag '-? prints usage' 0 'usage: diff' -?
checkdiag 'missing file is reported' 2 'cannot access' NOSUCH.TXT OLD.TXT
checkdiag 'only one argument is rejected' 2 'two filename' OLD.TXT
checkdiag 'no arguments is rejected' 2 'two filename'

echo
if [ $fail -eq 0 ]; then
	echo "all tests passed"
else
	echo "FAILURES"
fi
exit $fail
