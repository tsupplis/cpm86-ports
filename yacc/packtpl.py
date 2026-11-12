"""Prefix the template with YTPL and its exact unsigned 16-bit byte count."""
from pathlib import Path
import struct
import sys


def pack(source, destination):
    data = Path(source).read_bytes()
    if not 0 < len(data) <= 65529:
        raise ValueError('template must contain 1..65529 bytes')
    Path(destination).write_bytes(b'YTPL' + struct.pack('<H', len(data)) + data)


if __name__ == '__main__':
    pack(*sys.argv[1:])
