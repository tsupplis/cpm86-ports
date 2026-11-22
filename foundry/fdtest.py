"""Exercise the native CMD with scripted text-console input."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

def positions(output):
    """Replay tile writes in the well, yielding one set per visible change."""
    occupied = set()
    previous = set()
    for frame in output.split(b"\x1bY6 "):
        frame = re.sub(rb"\x1bj\x1bb.\x1bk", b"", frame)
        for row, col, tile in re.findall(rb"\x1bY(.)(.)([^\x1b\r\n]{2})", frame):
            x, y = col[0] - 38, row[0] - 36
            if 0 <= x < 16 and x % 2 == 0 and 0 <= y < 14:
                pos = (x // 2, y)
                if tile in (b"##", b"++", b"::", b"==", b"%%", b"**", b"oo", b"\xdb\xdb"):
                    occupied.add(pos)
                elif tile == b"  ":
                    occupied.discard(pos)
        if occupied != previous:
            previous = occupied.copy()
            yield previous


image = Path(__file__).resolve().with_name("foundry.cmd")
with tempfile.TemporaryDirectory(prefix="foundry-") as work:
    for version in ("2.2", "3.1", "4.1"):
        for mono in (False, True):
            args = ["-S"] + (["-M"] if mono else [])
            env = dict(os.environ, EMU2_CPM_VER=version, EMU2_CPM_VT52="0")
            result = subprocess.run(
                [sys.argv[1], "-c", "8086", str(image), *args], cwd=work,
                env=env, input=b"\x1bD\x1bC\x1bB\x1bA \x1b\n", stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, check=True, timeout=5,
            )
            out = result.stdout
            assert b"F O U N D R Y" in out and b"INCOMING" in out, out
            assert b"\xda" + b"\xc4" * 16 + b"\xbf" in out, out
            assert (b"\xdb\xdb" in out) != mono, out
            frames = list(positions(out))
            start = frames[0]
            assert 3 <= len(start) <= 5, out
            assert frames[1:5] == [
                {(x - 1, y) for x, y in start},
                start,
                {(x, y + 1) for x, y in start},
                {(3 - y, 1 + x - 3) for x, y in start},
            ], out
            assert out.count(b"Score ") == 3, out  # initial, drop, exit
            restore = b"\x1bm" if version == "2.2" else b"\x1be"
            assert restore + b"Foundry closed." in out, out
            inks = re.findall(rb"\x1bj\x1bb(.)\x1bk", out)
            if mono:
                assert not inks, out
            else:
                assert b"G" in inks and set(inks) - {b"G"}, out
                assert set(inks) <= {bytes((i,)) for i in (71, *range(73, 80))}, out
            print("PASS", version, "monochrome" if mono else "color")
