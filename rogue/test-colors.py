"""Validate rendered cells from real Aztec output in both terminal dialects."""
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
CC = shlex.split(os.environ.get('ROGUE_TEST_CC', 'aztec43_acc'))
MAKE = shlex.split(os.environ.get('ROGUE_TEST_MAKE', 'make'))


def run(command, wd):
    p = subprocess.run(command, cwd=wd, stdin=subprocess.DEVNULL,
                       capture_output=True, start_new_session=True, timeout=30)
    assert p.returncode == 0, (command, (p.stdout+p.stderr).decode('latin1'))
    return p.stdout


def render(data, dialect):
    row = column = 0
    saved = (0, 0)
    foreground = 7
    bold = reverse = False
    grid = [[(' ', 7, False) for _ in range(80)] for _ in range(24)]
    frames = {}
    index = 0
    while index < len(data):
        c = data[index]; index += 1
        if c == 30:  # out-of-band test snapshot marker
            end = data.index(31, index)
            frames[int(data[index:end])] = ([line[:] for line in grid],
                                            row, column, foreground, reverse)
            index = end+1
        elif c == 27:
            command = chr(data[index]); index += 1
            if dialect == 'VT52':
                if command == 'Y':
                    row, column = data[index]-32, data[index+1]-32; index += 2
                elif command == 'b':
                    foreground = data[index]; index += 1
                    assert 0 <= foreground < 16, foreground
                elif command == 'j': saved = row, column
                elif command == 'k': row, column = saved
                elif command == 'p': reverse = True
                elif command == 'q': reverse = False
                elif command == 'E':
                    grid = [[(' ', foreground, reverse) for _ in range(80)] for _ in range(24)]
                    row = column = 0
                elif command == 'A': row -= 1
                elif command == 'B': row += 1
                else: assert command in 'wv', command
            else:
                assert command == '[', command
                match = re.match(rb'([0-9;]*)([A-Za-z])', data[index:])
                assert match, data[index:index+30]
                index += match.end()
                args = [int(n or 0) for n in match[1].split(b';')]
                command = match[2]
                if command == b'm':
                    for value in args:
                        if value == 0: foreground = 7; bold = reverse = False
                        elif value == 1: bold = True
                        elif value == 7: reverse = True
                        elif 30 <= value <= 37:
                            foreground = [0, 4, 2, 6, 1, 5, 3, 7][value-30]
                        else: raise AssertionError(('SGR', value))
                    foreground = (foreground & 7) + (8 if bold else 0)
                elif command == b'H':
                    row = (args[0] or 1)-1
                    column = (args[1] or 1)-1 if len(args) > 1 else 0
                elif command == b'J':
                    assert args == [2]
                    grid = [[(' ', foreground, reverse) for _ in range(80)] for _ in range(24)]
                elif command == b'A': row -= args[0] or 1
                elif command == b'B': row += args[0] or 1
                else: raise AssertionError(command)
        elif c == 8: column -= 1
        elif c == 13: column = 0
        elif c == 10: row += 1
        else:
            assert 32 <= c < 127, ('unexpected control byte', c)
            assert 0 <= row < 24 and 0 <= column < 80, (row, column)
            grid[row][column] = (chr(c), foreground, reverse)
            column += 1
    return frames


def check(frames):
    assert set(frames) == set(range(1, 13))
    symbols = '.A@!-|+?*:/])=,'
    palette = [1, 12, 11, 10, 14, 14, 14]+[10]*8
    expected = list(zip(symbols, palette, [False]*len(symbols)))
    for n in (1, 3, 4, 7, 10):
        assert frames[n][0][4][10:25] == expected, (n, frames[n][0][4][10:25])
    for n in (1, 7):
        for row, text in [(0, 'A hero @ . - !'), (23, 'STATUS A @ !')]:
            assert frames[n][0][row][:len(text)] == [(c, 7, False) for c in text]
    for n in (2, 8): assert frames[n][0][4][11] == ('A', 7, False)
    assert frames[4][1:3] == (4, 14), frames[4][1:3]
    assert frames[5][0][4][14] == ('-', 14, True)
    assert frames[5][0][4][15] == ('|', 14, False)
    assert frames[6][0][4][10:12] == expected[:2]
    assert all(cell == (' ', 7, False) for cell in frames[6][0][4][12:])
    assert frames[9][0][4][11] == ('Z', 7, False)
    assert all(cell == (' ', 7, False) for row in frames[11][0] for cell in row)
    assert frames[12][0][4][10] == ('@', 11, True)
    assert all(frame[3:] == (7, False) for frame in frames.values())


with tempfile.TemporaryDirectory(prefix='rogue-colors-') as directory:
    base = Path(directory)
    results = []
    for dialect, define in [('VT52', '-DVT52'), ('VT100', '-DANSI')]:
        wd = base/dialect; wd.mkdir()
        for name in ('curses.c', 'rogue.h', 'fmt.c', 'testdisp.c'):
            shutil.copy2(ROOT/name, wd/name)
        run(CC+['-target=cpm86', '-I.', '-DCURSES', define, '-squeeze',
                '-o', 'display.cmd', 'testdisp.c', 'curses.c', 'fmt.c'], wd)
        output = run(['emu2', 'display.cmd'], wd)
        frames = render(output, dialect)
        check(frames)
        results.append(frames)
        print(dialect+': palette, text, reverse video, cursor replay, redraw, clear and exit PASS')
    assert results[0] == results[1], 'terminal dialects produce different screens'

    # Switching dialect must also relink the root and matching overlays,
    # even when two builds happen in the same filesystem timestamp tick.
    wd = base/'switch'; wd.mkdir()
    for path in ROOT.iterdir():
        if path.is_file() and (path.suffix in ('.c', '.h', '.o', '.cmd', '.ovr', '.rsm')
                               or path.name in ('Makefile', 'terminal.cfg')):
            shutil.copy2(path, wd/path.name)
    initial = (wd/'terminal.cfg').read_text().strip()
    images = []
    for dialect in ['VT100' if initial == 'VT52' else 'VT52', initial]:
        output = run(MAKE+['all', 'TERMINAL='+dialect, 'CC='+shlex.join(CC),
                          'LD='+shlex.join(CC)], wd).decode('latin1')
        assert ('-DVT52' if dialect == 'VT52' else '-DANSI') in output, output
        assert '-o rogue.cmd' in output, output
        for overlay in ('init', 'save', 'score'):
            assert '-o '+overlay+'.ovr' in output, output
            assert (wd/(overlay+'.ovr')).stat().st_size > 0
        images.append((wd/'rogue.cmd').read_bytes())
    assert images[0] != images[1], 'terminal switch did not change the executable'
print('PASS: VT52 and VT100 produce identical colored frames')
print('PASS: terminal switching rebuilds the complete game and all overlays')
