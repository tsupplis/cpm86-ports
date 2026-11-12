"""Validate replacement and rollback on separate CP/M drives."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent
for mode in range(6):
    with tempfile.TemporaryDirectory() as folder:
        wd = Path(folder)
        drive = wd / 'b'
        drive.mkdir()
        (wd / 'BASE.DAT').write_bytes(b'DEFAULT DRIVE')
        (drive / 'BASE.DAT').write_bytes(b'OLD')
        (drive / 'FS0000.TMP').write_bytes(b'OCCUPIED')
        env = dict(os.environ, EMU2_DRIVE_B=str(drive))
        p = subprocess.run(['emu2', str(root / 'tests/testsave.cmd'), str(mode)],
                           cwd=wd, env=env, stdin=subprocess.DEVNULL,
                           capture_output=True, start_new_session=True, timeout=10)
        text = (p.stdout+p.stderr).decode('latin1')
        assert 'SAVE PASS' in text and 'FAIL:' not in text, text
        assert (wd / 'BASE.DAT').read_bytes() == b'DEFAULT DRIVE'
        assert (drive / 'FS0000.TMP').read_bytes() == b'OCCUPIED'
        contents = [f.read_bytes().rstrip(b'\0\x1a') for f in drive.iterdir()]
        if mode:
            assert b'OLD' in contents, (mode, contents)
        assert b'NEW' in contents, (mode, contents)
print('Replacement: success, backup, rename failures, rollback failure and collision PASS')
