#!/usr/bin/env python3
"""Pack the five example sources into 64-column fig-FORTH screens."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCES = ('gcd.fth', 'factor.fth', 'fib.fth', 'primes.fth', 'stars.fth')


def main():
    screens = [b' ' * 1024]  # screen zero is reserved for terminal input
    for name in SOURCES:
        lines = (ROOT / name).read_text(encoding='ascii').splitlines()
        if len(lines) > 16 or any(len(line) > 64 for line in lines):
            raise SystemExit(f'{name}: a screen allows 16 lines of 64 characters')
        screen = b''.join(line.encode('ascii').ljust(64, b' ') for line in lines)
        screens.append(screen.ljust(1024, b' '))
    (ROOT / 'demo.blk').write_bytes(b''.join(screens))


if __name__ == '__main__':
    main()
