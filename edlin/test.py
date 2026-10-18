"""Exercise EDLIN's real allocation limit without PCE or a host terminal."""
from pathlib import Path
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parent
image = (root / 'edlin.cmd').read_bytes()


def run(folder, limit, commands):
    cmd = bytearray(image)
    # emu2 grants 8080 programs at least 64 KB. A test-only startup stub
    # supplies a smaller base-page allocation before the real entry code.
    # Place it in the unused copyright banner; the shipped CMD is unchanged.
    entry = 128 + 0x100
    stub_at = image.index(b'\r\nThe IBM Personal Computer EDITOR')
    stub_offset = stub_at - 128
    stub = image[entry:entry + 6]
    stub += b'\xc7\x06\x00\x00' + struct.pack('<H', limit & 0xffff)
    stub += b'\xc6\x06\x02\x00' + bytes([limit >> 16])
    stub += b'\xe9' + struct.pack('<h', 0x106 - (stub_offset + len(stub) + 3))
    cmd[stub_at:stub_at + len(stub)] = stub
    cmd[entry:entry + 3] = b'\xe9' + struct.pack('<h', stub_offset - 0x103)
    (folder / 'EDITOR.CMD').write_bytes(cmd)
    return subprocess.run(
        ['emu2', str(folder / 'EDITOR.CMD'), 'EDIT.TXT'],
        input=commands, cwd=folder, capture_output=True,
        start_new_session=True, timeout=10)


def contents(folder, name):
    return next(p for p in folder.iterdir() if p.name.upper() == name).read_bytes()


for limit in (65536, 16384, 8192):
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        original = b''.join(('line %04d: vintage editor\r\n' % i).encode()
                            for i in range(1200))
        (folder / 'EDIT.TXT').write_bytes(original + b'\x1a')
        result = run(folder, limit, b'1I\ninserted\n\x1a\nE\n')
        assert result.returncode == 0, result.stderr
        assert b'Insufficient memory' not in result.stdout, result.stdout
        assert contents(folder, 'EDIT.TXT').split(b'\x1a')[0] == b'inserted\r\n' + original
        assert contents(folder, 'EDIT.BAK') == original + b'\x1a'
        assert not any(p.suffix == '.$$$' for p in folder.iterdir())
        print('EDLIN: insert/save and multi-buffer preservation at %d KB passed' % (limit // 1024))

with tempfile.TemporaryDirectory() as directory:
    folder = Path(directory)
    original = b'keep this file\r\n\x1a'
    (folder / 'EDIT.TXT').write_bytes(original)
    result = run(folder, 4096, b'')
    assert result.returncode == 0, result.stderr
    assert b'Insufficient memory' in result.stdout, result.stdout
    assert contents(folder, 'EDIT.TXT') == original
    assert sorted(p.name for p in folder.iterdir()) == ['EDIT.TXT', 'EDITOR.CMD']
    print('EDLIN: insufficient allocation exits without file changes passed')
