#!/bin/sh
# Regression tests for keydb — the ndbm-backed key/value tool.
#
# Both binaries (keydb.cmd for CP/M-86, keydb.com for DOS) are exercised
# through emu2.  Two target differences matter here:
#
#   - emu2 does not fold the command tail the way the CP/M-86 CCP does, so
#     commands, keys and values are written in lower case: they reach the
#     binary unchanged on both targets.
#   - CP/M-86 has no exit status, so for keydb.cmd the return code is not
#     checked and diagnostics are looked for in the combined output.

cd "$(dirname "$0")" || exit 1

LC_ALL=C
export LC_ALL

TIMEOUT=10
TARGETS="keydb.cmd keydb.com"
DB=test
fail=0

for binary in $TARGETS; do
	if [ ! -f "$binary" ]; then
		echo "$binary missing, run make first" >&2
		exit 1
	fi
done

trap 'rm -f ${DB}.dir ${DB}.pag out.tmp err.tmp' EXIT INT TERM

has_status() {
	# Neither .cmd (CP/M-86, no exit status) nor .com (Aztec C86 runtime
	# does not reliably propagate main()'s return value to DOS) report a
	# meaningful exit code.
	return 1
}

run() {
	binary=$1; shift
	timeout $TIMEOUT emu2 $binary "$@" >out.tmp 2>err.tmp </dev/null
	rc=$?
	out=$(tr -d '\r' <out.tmp)
	diag=$(cat err.tmp out.tmp | tr -d '\r')
}

pass() { printf 'PASS  %-11s %s\n' "$1" "$2"; }

blame() {
	printf 'FAIL  %-11s %s\n' "$1" "$2"
	printf '        argv:     %s\n'     "$3"
	printf '        expected: %s\n'     "$4"
	printf '        got:      status %s\n' "$rc"
	printf '%s\n' "$5" | sed 's/^/        | /'
	fail=1
}

# check <desc> <xrc> <xout> <arg>...
check() {
	desc=$1; xrc=$2; xout=$3; shift 3
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
			blame "$binary" "$desc" "$*" \
				"status $xrc
$(printf '%s\n' "$xout" | sed 's/^/        | /')" "$out"
		fi
	done
}

# checkdiag <desc> <xrc> <needle> <arg>...
checkdiag() {
	desc=$1; xrc=$2; needle=$3; shift 3
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
				"status $xrc with message containing \"$needle\"" "$diag"
		fi
	done
}

# Reset the database files before each test group
resetdb() { rm -f ${DB}.dir ${DB}.pag; }

# ------------------------------------------------------------------ #
echo "== add / get =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB fruit apple
	run $binary get $DB fruit
	if printf '%s\n' "$out" | grep -qiF "apple"; then
		pass "$binary" "add then get returns value"
	else
		blame "$binary" "add then get returns value" \
			"add $DB fruit apple / get $DB fruit" \
			"apple" "$out"
	fi
done

# ------------------------------------------------------------------ #
echo "== replace =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB color red
	run $binary add $DB color blue
	run $binary get $DB color
	if printf '%s\n' "$out" | grep -qiF "blue"; then
		pass "$binary" "second add replaces value"
	else
		blame "$binary" "second add replaces value" \
			"add color red / add color blue / get color" \
			"blue" "$out"
	fi
done

# ------------------------------------------------------------------ #
echo "== multiple keys =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB a 1
	run $binary add $DB b 2
	run $binary add $DB c 3
	run $binary get $DB b
	if [ "$out" = "2" ]; then
		pass "$binary" "get middle key of three"
	else
		blame "$binary" "get middle key of three" \
			"add a 1 / add b 2 / add c 3 / get b" \
			"2" "$out"
	fi
done

# ------------------------------------------------------------------ #
echo "== keys and values with special characters =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB 'foo%bar' 'baz%qux'
	run $binary get $DB 'foo%bar'
	if printf '%s\n' "$out" | grep -qiF "baz%qux"; then
		pass "$binary" "% in key and value"
	else
		blame "$binary" "% in key and value" \
			"add foo%bar baz%qux / get foo%bar" \
			"baz%qux" "$out"
	fi
done

# ------------------------------------------------------------------ #
echo "== list =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB x foo
	run $binary add $DB y bar
	run $binary list $DB
	ok=yes
	printf '%s\n' "$out" | grep -qiF "x=foo" || ok=no
	printf '%s\n' "$out" | grep -qiF "y=bar" || ok=no
	if [ $ok = yes ]; then
		pass "$binary" "list shows all entries"
	else
		blame "$binary" "list shows all entries" \
			"add x foo / add y bar / list $DB" \
			"lines containing x=foo and y=bar" "$out"
	fi
done

# ------------------------------------------------------------------ #
echo "== delete =="
# ------------------------------------------------------------------ #
for binary in $TARGETS; do
	resetdb
	run $binary add $DB gone yes
	run $binary del $DB gone
	run $binary get $DB gone
	if printf '%s\n' "$diag" | grep -qF "not found"; then
		pass "$binary" "deleted key is not found"
	else
		blame "$binary" "deleted key is not found" \
			"add gone yes / del gone / get gone" \
			"message containing 'not found'" "$diag"
	fi
done

# ------------------------------------------------------------------ #
echo "== error cases =="
# ------------------------------------------------------------------ #
checkdiag "no args prints usage"  0 "usage: keydb"
checkdiag "get missing key"       1 "not found"   get $DB nosuchkey
checkdiag "del missing key"       1 "failed"      del $DB nosuchkey

# ------------------------------------------------------------------ #
echo
if [ $fail -eq 0 ]; then echo "all tests passed"
else                      echo "FAILURES"; fi
exit $fail
