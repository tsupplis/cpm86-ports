#!/usr/bin/env python3
"""Native fig-FORTH regressions under emu2; no PCE or external Python packages."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
EMU = shutil.which(os.environ.get('EMU2', 'emu2'))
if not EMU:
    raise SystemExit('emu2 not found; set EMU2 to its executable path')


def run(work, lines, expected=(), version='2.2', poison=False):
    script = ('\r'.join(lines) + '\r').encode('ascii')
    keys = work / 'keys.txt'
    keys.write_bytes(script)
    env = dict(os.environ, EMU2_CPM_VER=version, EMU2_DRIVE_C=str(work),
               EMU2_DEFAULT_DRIVE='C:')
    command = [EMU, '-c', '8086', '-s', str(keys)]
    if poison:
        command += ['-D']
    command += [str(ROOT / 'forth.cmd')]
    result = subprocess.run(command, input=script, cwd=work, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=10)
    output = result.stdout.decode('ascii', 'replace').replace('\r', '')
    assert result.returncode == 0, output
    assert 'unimplemented' not in output and 'error,' not in output, output
    for text in expected:
        assert text in output, (text, output)
    return output


def suite(version):
    with tempfile.TemporaryDirectory(prefix='figforth-') as directory:
        w = Path(directory)
        run(w, ['1 2 + .', '7 3 /MOD . .', '-12 5 + .', 'BYE'],
            ['3 OK', '2 1 OK', '-7 OK'], version, True)
        run(w, ['10 20 SWAP OVER + . .', ': SQUARE DUP * ;', '12 SQUARE .',
                ': SUM 0 5 0 DO I + LOOP ;', 'SUM .',
                ': CHOOSE IF 111 ELSE 222 ENDIF ;', '0 CHOOSE . 1 CHOOSE .',
                '0 VARIABLE VALUE', '321 VALUE ! VALUE @ .', 'BYE'],
            ['30 20 OK', '144 OK', '10 OK', '222 111 OK', '321 OK'], version)
        run(w, [': MAKER <BUILDS , DOES> @ ;', '777 MAKER SEVEN', 'SEVEN .',
                ': TEMP 888 ;', 'TEMP .', 'FORGET TEMP', '3 4 + .', 'BYE'],
            ['777 OK', '888 OK', '7 OK'], version)
        run(w, ['NOTAWORD', '6 7 * .', 'BYE'], ['?', '42 OK'], version)
        run(w, ['8 BLOCK C@ .', '65 8 BLOCK C! UPDATE', 'FLUSH', 'BYE'],
            ['32 OK'], version)
        blockfile = w / 'forth.blk'
        if not blockfile.exists():
            blockfile = w / 'FORTH.BLK'
        assert blockfile.read_bytes()[8 * 128] == 65
        assert blockfile.read_bytes()[:8 * 128] == b' ' * (8 * 128)
        run(w, ['8 BLOCK C@ .', '66 9 BLOCK C! UPDATE', 'BYE'],
            ['65 OK'], version)
        assert blockfile.read_bytes()[9 * 128] == 66, 'BYE must flush changes'
        run(w, ['9 BLOCK C@ .', 'BYE'], ['66 OK'], version)
        run(w, ['67 127 BLOCK C! UPDATE', '68 128 BLOCK C! UPDATE',
                '69 255 BLOCK C! UPDATE', '70 256 BLOCK C! UPDATE', 'BYE'],
            version=version)
        run(w, ['127 BLOCK C@ . 128 BLOCK C@ .',
                '255 BLOCK C@ . 256 BLOCK C@ .', 'BYE'],
            ['67 68 OK', '69 70 OK'], version)
        # A source screen is eight 128-byte records, i.e. sixteen 64-byte lines.
        data = bytearray(blockfile.read_bytes())
        data.extend(b' ' * max(0, 3072 - len(data)))
        source = b': CUBE DUP DUP * * ;'
        data[2048:2048 + len(source)] = source
        blockfile.write_bytes(data)
        run(w, ['2 LOAD', '5 CUBE .', 'BYE'], ['125 OK'], version)
        # Exercise the shipped examples through LOAD, not a second copy.
        blockfile.write_bytes((ROOT / 'examples/demo.blk').read_bytes())
        run(w, ['1 LOAD', '2 LOAD', '3 LOAD', '4 LOAD', '5 LOAD', 'BYE'],
            ['6 OK', '5040 OK', '0 1 1 2 3 5 8 13 21 34 55 OK',
             '2 3 5 7 11 13 17 19 23 29 OK', '*\n**\n***\n****\n*****'],
            version)
        run(w, ['8192 BLOCK DROP'], ['FORTH.BLK I/O error'], version)
    print('PASS: CP/M version', version,
          '- arithmetic, stacks, definitions, loops, branches, variables,',
          'DOES>, FORGET, error recovery, blank blocks, persistence, extent boundaries, BYE flush, LOAD, bounds')


suite('2.2')
suite('3.1')
