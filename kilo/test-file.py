"""Target-runtime load/save regressions, including memory and I/O failure."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent
for mode in [0, 1, 2, 3, *range(10, 22)]:
    with tempfile.TemporaryDirectory() as folder:
        wd = Path(folder)
        drive = wd / 'b'
        drive.mkdir()
        target = drive / 'EDIT.TXT'
        original = b'X'*192 if mode==19 else b'first\r\nlast\x1aSTALE\r\nTAIL'
        target.write_bytes(original)
        (wd / 'EDIT.TXT').write_bytes(b'DEFAULT DRIVE')
        p = subprocess.run(['emu2', str(root / 'tests/testfile.cmd'), str(mode)],
                           cwd=wd, env=dict(os.environ, EMU2_DRIVE_B=str(drive)),
                           stdin=subprocess.DEVNULL, capture_output=True,
                           start_new_session=True, timeout=10)
        text = (p.stdout+p.stderr).decode('latin1')
        assert 'KILO PASS' in text and 'FAIL:' not in text, (mode, text)
        if mode:
            assert target.read_bytes() == original, mode
        else:
            assert target.read_bytes().split(b'\x1a', 1)[0] == b'saved\r\nlast\r\n'
        assert (wd / 'EDIT.TXT').read_bytes() == b'DEFAULT DRIVE'

# Exercise the real startup path: a failed load must exit before editing.
with tempfile.TemporaryDirectory() as folder:
    wd = Path(folder)
    original = b'X' * 33000 + b'\r\n'
    (wd / 'LARGE.TXT').write_bytes(original)
    p = subprocess.run(['emu2', str(root / 'kilo.cmd'), 'LARGE.TXT'], cwd=wd,
                       stdin=subprocess.DEVNULL, capture_output=True,
                       start_new_session=True, timeout=10)
    assert b'Cannot load complete file' in p.stdout+p.stderr
    assert (wd / 'LARGE.TXT').read_bytes() == original
print('Kilo: streaming save, close/write/rename failure, 10 allocation failures, read failure and failed startup PASS')
