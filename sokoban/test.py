"""CP/M embedded resources/game help and the DOS file-backed alternative."""
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
CC = shlex.split(os.environ.get('SK_TEST_CC', 'aztec43_acc'))
CMDMOD = shlex.split(os.environ.get('SK_TEST_CMDMOD', 'cmdmod'))
MAKE = shlex.split(os.environ.get('SK_TEST_MAKE', 'make'))
NAMES = ('soklevls.dat', 'sokoban.hlp')
DATA = [ (ROOT/name).read_bytes() for name in NAMES ]


def run(command, wd):
    result = subprocess.run(command, cwd=wd, stdin=subprocess.DEVNULL,
                            capture_output=True, start_new_session=True, timeout=30)
    output = (result.stdout+result.stderr).decode('latin1')
    assert result.returncode == 0 and 'FAIL:' not in output, (command, output)
    return output


def digest(data):
    value = 0
    for byte in data:
        value = (value*33+byte) & 65535
    return value


def group(image):
    assert [image[i*9] for i in range(4)] == [1, 2, 3, 0]
    offset = 128+sum(struct.unpack_from('<H', image, 9*i+1)[0]*16 for i in range(2))
    size = struct.unpack_from('<H', image, 19)[0]*16
    assert struct.unpack_from('<HH', image, 23) == (size//16, size//16)
    return offset, size


def check_image(image, data):
    offset, size = group(image)
    assert image[offset:offset+4] == b'SKR1'
    assert struct.unpack_from('<HHHH', image, offset+4) == (
        12, len(data[0]), 12+len(data[0]), len(data[1]))
    assert image[offset+12:offset+12+sum(map(len, data))] == b''.join(data)


def embed(binary, payload, wd):
    run(CMDMOD + ['-t', 'EXTRA', '-s', str(payload),
                  '-m', format(payload.stat().st_size, 'x'), str(binary), '2'], wd)


def scan(binary, wd):
    output = run(['emu2', str(binary)], wd)
    assert 'RESOURCE PASS' in output, output
    assert re.findall(r'RESOURCE (\d+) (\d+) (\d+)', output) == [
        (str(i), str(len(data)), str(digest(data))) for i, data in enumerate(DATA)]


check_image((ROOT/'sokoban.cmd').read_bytes(), DATA)
with tempfile.TemporaryDirectory(prefix='sokoban-resources-') as folder:
    base = Path(folder)
    build = base/'build'; build.mkdir()
    for name in ('sokoban.c', 'config.h', 'gameio.c', 'gameio.h', 'conio.h',
                 'util.h', 'util.lib', 'resource.c', 'resource.h', 'help.c',
                 'help.h', 'testgame.c', 'testres.c'):
        shutil.copy2(ROOT/name, build/name)
    run(CC+['-target=cpm86', '-I.', '-squeeze', '-o', 'tgame.cmd',
            'testgame.c', 'resource.c', 'gameio.c', 'util.lib'], build)
    run(CC+['-target=cpm86', '-I.', '-squeeze', '-o', 'tres.cmd',
            'testres.c', 'resource.c'], build)
    for binary in ('tgame.cmd', 'tres.cmd'):
        embed(build/binary, ROOT/'res.bin', build)

    # These run directories contain neither external resource file.
    wd = base/'memory'; wd.mkdir()
    scan(build/'tres.cmd', wd)
    output = run(['emu2', str(build/'tgame.cmd'), 'levels', '50'], wd)
    assert 'LEVELS PASS' in output, output
    expected = []
    for i, chunk in enumerate(DATA[0].split(b'\f')[:-1]):
        if i: chunk = chunk[1:]  # newline following the preceding form feed
        lines = chunk.splitlines()
        player = next((row, column) for row, line in enumerate(lines)
                      for column, byte in enumerate(line) if byte in b'@+')
        expected.append(tuple(map(str, (i+1, len(lines), max(map(len, lines)),
                         chunk.count(b'$')+chunk.count(b'*'), chunk.count(b'*'),
                         *player, digest(chunk)))))
    assert re.findall(r'LEVEL (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)', output) == expected

    for mode in ('help', 'undo', 'quit', 'escape'):
        output = run(['emu2', str(build/'tgame.cmd'), mode], wd)
        assert 'GAME PASS' in output and 'Movement:' in output, output
        assert ('TEMPORARY CHECKPOINTS' in output) == (mode in ('help', 'undo'))
        assert '(PAGE BREAK)' not in output and '(END)' not in output

    image = (build/'tres.cmd').read_bytes()
    offset, size = group(image)
    padded = bytearray(image)
    end = offset+12+sum(map(len, DATA))
    padded[end:offset+size] = b'\xff'*(offset+size-end)
    (wd/'PAD.CMD').write_bytes(padded)
    scan(wd/'PAD.CMD', wd)

    for case in ('missing', 'magic', 'overlap', 'length', 'empty'):
        broken = base/case; broken.mkdir()
        image = bytearray((build/'tgame.cmd').read_bytes())
        offset, _ = group(image)
        if case == 'magic': image[offset] = 0
        if case == 'overlap': struct.pack_into('<H', image, offset+8, 12)
        if case == 'length': struct.pack_into('<H', image, offset+10, 65535)
        if case == 'empty': struct.pack_into('<H', image, offset+10, 0)
        (broken/'BAD.CMD').write_bytes(image)
        if case == 'missing': run(CMDMOD+['-d', 'BAD.CMD', '2'], broken)
        output = run(['emu2', 'BAD.CMD', 'missing'], broken)
        assert 'MISSING PASS' in output and 'Help is unavailable' in output, output

    dos = base/'dos'; dos.mkdir()
    for name in ('resource.c', 'resource.h', 'testres.c'):
        shutil.copy2(ROOT/name, dos/name)
    run(CC+['-target=dos1', '-I.', '-squeeze', '-o', 'tres.com',
            'testres.c', 'resource.c'], dos)
    assert 'MISSING PASS' in run(['emu2', 'tres.com', 'missing'], dos)
    for name, data in zip(NAMES, DATA): (dos/name).write_bytes(data)
    scan(dos/'tres.com', dos)

    for i, name in enumerate(NAMES):
        rebuild = base/('rebuild'+str(i)); rebuild.mkdir()
        for path in ROOT.iterdir():
            if path.is_file() and (path.suffix in ('.c', '.h', '.o', '.cmd', '.bin', '.lib', '.dat', '.hlp')
                                   or path.name in ('Makefile', 'packres.py')):
                shutil.copy2(path, rebuild/path.name)
        updated = list(DATA)
        updated[i] += b'\n'
        (rebuild/name).write_bytes(updated[i])
        # Make the one changed source newer even with second-resolution make.
        stamp = max((rebuild/'res.bin').stat().st_mtime,
                    (rebuild/'sokoban.cmd').stat().st_mtime) + 2
        os.utime(rebuild/name, (stamp, stamp))
        run(MAKE+['sokoban.cmd', 'CC='+shlex.join(CC),
                  'LD='+shlex.join(CC), 'CMDMOD='+shlex.join(CMDMOD)], rebuild)
        check_image((rebuild/'sokoban.cmd').read_bytes(), updated)

print('PASS: 50 embedded levels, both help pages, Q/Esc return, board/undo/checkpoint preservation')
print('PASS: exact resource bounds, invalid groups, DOS file reader and both resource rebuilds')
