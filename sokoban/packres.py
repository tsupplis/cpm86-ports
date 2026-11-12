"""Pack the two read-only Sokoban resources into one bounded CMD group."""
from pathlib import Path
import struct
import sys


def pack(levels_path, help_path, output):
    levels = Path(levels_path).read_bytes()
    help_text = Path(help_path).read_bytes()
    if not levels or not help_text or 12+len(levels)+len(help_text) > 65535:
        raise ValueError('nonempty levels and help must fit in one 64 KiB group')
    header = b'SKR1'+struct.pack('<HHHH', 12, len(levels),
                                12+len(levels), len(help_text))
    Path(output).write_bytes(header+levels+help_text)


if __name__ == '__main__':
    pack(*sys.argv[1:])
