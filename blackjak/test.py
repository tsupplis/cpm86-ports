"""Run deterministic rule checks and smoke-test both shipped binaries."""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    cc, emu = sys.argv[1:]
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="blackjac-") as temp:
        check = Path(temp) / "bjtest"
        subprocess.run(
            [cc, "-std=c89", "-Wall", "-Wextra", "-Werror",
             "rules.c", "bjtest.c", "-o", str(check)],
            cwd=root, check=True, timeout=30,
        )
        subprocess.run([str(check)], check=True, timeout=5)
        print("PASS scoring, dealer policy, actions, payouts and deck integrity")
        cases = [
            ("blackjak.cmd", {"EMU2_CPM_VER": version})
            for version in ("2.2", "3.1", "4.1")
        ] + [("blackjak.com", {})]
        for binary, profile in cases:
            run = subprocess.run(
                [emu, "-c", "8086", str(root / binary)],
                cwd=temp, env={**os.environ, "EMU2_CPM_VT52": "0", **profile},
                input=b"501\n1 2\n7777\n5\n0\n0\n0\n",
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                check=True, timeout=5,
            )
            out = run.stdout
            assert out.count(b"Enter a whole-dollar wager") == 2, out
            assert out.count(b"Deck shuffled.") == 1, out
            assert out.count(b"Balance:") == 1, out
            color = rb"\x1bj\x1bb([\x07\x09\x0c])\x1bk"
            faces = rb"\xb3(?: [A2-9JQK]|10)([SDHC]) \xb3"
            if profile:
                colored = re.findall(color + faces + color, out)
                assert len(colored) == len(re.findall(faces, out)) > 0, out
                for ink, suit, reset in colored:
                    assert ink == (b"\x0c" if suit in (b"D", b"H") else b"\x09"), out
                    assert reset == b"\x07", out
                assert not re.search(color + rb"\xb3####\xb3", out), out
            else:
                assert not re.search(color, out), out
            screen = re.sub(color, b"", out).decode("cp437")
            assert "┌────┐ ┌────┐" in screen, screen
            shown, revealed = screen.split("Dealer reveals", 1)
            assert shown.count("│####│") == 1, screen
            assert "│####│" not in revealed and "┌────┐ ┌────┐" in revealed, screen
            print("PASS", binary, profile.get("EMU2_CPM_VER", "DOS"))


if __name__ == "__main__":
    main()
