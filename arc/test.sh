#!/bin/sh
# Functional tests for ARC on CP/M-86, run under emu2.
# CP/M-86 stores sizes in 128 byte records, so an extracted file may carry
# zero padding up to the next record boundary: compare the original as a
# prefix and require only NULs after it.

here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp "$here/arc.cmd" "$here/marc.cmd" "$work/"
cd "$work" || exit 1

fails=0
checks=0

run() {
	timeout 60 emu2 "$@" </dev/null 2>&1 | tr -d '\r'
}

check() {		# check "description" command...
	desc=$1; shift
	checks=$((checks+1))
	if "$@"; then
		:
	else
		echo "FAIL: $desc"
		fails=$((fails+1))
	fi
}

same() {		# same orig extracted: prefix match, NUL padding only
	python3 - "$1" "$2" <<'EOF'
import sys
o = open(sys.argv[1], 'rb').read()
x = open(sys.argv[2], 'rb').read()
ok = x.startswith(o) and not x[len(o):].strip(b'\0') and len(x) - len(o) < 128
sys.exit(0 if ok else 1)
EOF
}

has() {			# has "text" file
	grep -q "$1" "$2"
}

python3 - <<'EOF'
open('TEXT.TXT', 'wb').write(b'hello hello hello world\r\n' * 400)
open('BIN.DAT', 'wb').write(bytes(range(256)) * 20)
open('ONE.DAT', 'wb').write(b'X' * 1000)
open('RAND.DAT', 'wb').write(bytes((i * 7919 + (i >> 3) * 31) & 255 for i in range(4096)))
EOF
mkdir orig && cp TEXT.TXT BIN.DAT ONE.DAT RAND.DAT orig/

# add several files, list, test
run arc.cmd a T text.txt bin.dat one.dat rand.dat > add.log
check "add reports files" has "Adding file" add.log
run arc.cmd v T > list.log
for f in TEXT.TXT BIN.DAT ONE.DAT RAND.DAT; do
	check "list shows $f" has "$f" list.log
done
run arc.cmd t T > test.log
check "integrity test clean" has "No errors detected" test.log

# extract everything elsewhere and compare
mkdir out && cp T.ARC out/ && (cd out && run ../arc.cmd x T > ../x.log)
for f in TEXT.TXT BIN.DAT ONE.DAT RAND.DAT; do
	lc=$(echo "$f" | tr 'A-Z' 'a-z')
	check "round trip $f" same "orig/$f" "out/$lc"
done

# extract by name only
mkdir out2 && cp T.ARC out2/ && (cd out2 && run ../arc.cmd x T one.dat > ../x2.log)
check "extract one file" same orig/ONE.DAT out2/one.dat
check "extract one file only" test "$(ls out2 | wc -l)" -eq 2

# wildcard add
mkdir w && cp orig/TEXT.TXT orig/BIN.DAT orig/ONE.DAT w/
(cd w && run ../arc.cmd a W '*.dat' > ../w.log; run ../arc.cmd v W > ../wl.log)
check "wildcard adds BIN" has "BIN.DAT" wl.log
check "wildcard adds ONE" has "ONE.DAT" wl.log
check "wildcard skips TEXT" sh -c '! grep -q TEXT.TXT wl.log'

# delete
cp T.ARC D.ARC
run arc.cmd d D bin.dat > d.log
run arc.cmd v D > dl.log
check "delete removed BIN" sh -c '! grep -q BIN.DAT dl.log'
check "delete kept TEXT" has "TEXT.TXT" dl.log

# store only
run arc.cmd as S one.dat > s.log
run arc.cmd v S > sl.log
check "store only" has "ONE.DAT *1024 *-- *0%" sl.log

# overwrite extract
mkdir out3 && cp T.ARC out3/ && cp orig/ONE.DAT out3/one.dat
(cd out3 && run ../arc.cmd xo T one.dat > ../o.log)
check "overwrite extract" same orig/ONE.DAT out3/one.dat

# squash is refused rather than crashing
run arc.cmd aq Q one.dat > q.log
check "squash refused" has "CP/M-86" q.log

# marc merges archives
run arc.cmd a A1 one.dat > /dev/null
run arc.cmd a A2 bin.dat > /dev/null
run marc.cmd A1 A2 > m.log
run arc.cmd v A1 > ml.log
check "marc merged BIN" has "BIN.DAT" ml.log
check "marc kept ONE" has "ONE.DAT" ml.log

echo "$((checks-fails))/$checks checks passed"
exit $((fails > 0))
