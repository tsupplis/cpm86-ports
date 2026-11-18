#!/usr/bin/env python3
"""mkinstr.py -- compile instr.txt into instr.bin, the tinasm instruction table.

Input lines are NAME|operands|encoding ('#' lines and blank lines ignored).
Entry order is preserved (first match wins in tinasm).

instr.bin layout (little endian, offsets from the start of the image):
    +0              word  count
    +2              word  offset[0] .. offset[count-1]
    offset[i]       NAME 0 operands 0 encoding 0
The image is added to tinasm.cmd as the EXTRA group with cmdmod and read
with the cmdseg API.
"""
import struct
import sys

MAX_ENTRIES = 320   # must match MAX_INSTR in tinasm.c
MAX_ENTRY = 63      # entry bytes incl. NULs; tinasm reads into char[64]


def main(src, dst):
    entries = []
    for n, line in enumerate(open(src), 1):
        line = line.rstrip("\r\n")
        if not line or line.startswith("#"):
            continue
        f = line.split("|")
        if len(f) != 3 or not f[0]:
            sys.exit("%s:%d: expected NAME|operands|encoding" % (src, n))
        e = "\0".join(f).encode("ascii") + b"\0"
        if len(e) > MAX_ENTRY:
            sys.exit("%s:%d: entry too long (%d)" % (src, n, len(e)))
        entries.append(e)
    if len(entries) > MAX_ENTRIES:
        sys.exit("too many entries (%d)" % len(entries))
    off = 2 + 2 * len(entries)
    index = b""
    for e in entries:
        index += struct.pack("<H", off)
        off += len(e)
    with open(dst, "wb") as out:
        out.write(struct.pack("<H", len(entries)) + index + b"".join(entries))
    print("%s: %d entries, %d bytes" % (dst, len(entries), off))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: mkinstr.py instr.txt instr.bin")
    main(sys.argv[1], sys.argv[2])
