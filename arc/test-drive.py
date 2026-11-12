"""ARC and MARC must replace archives on the explicitly selected drive."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory() as folder:
    wd = Path(folder)
    drive = wd / 'b'
    drive.mkdir()
    env = dict(os.environ, EMU2_DRIVE_B=str(drive))

    def run(binary, *args):
        p = subprocess.run(['emu2', str(root / (binary+'.cmd')), *args],
                           cwd=wd, env=env, stdin=subprocess.DEVNULL,
                           capture_output=True, start_new_session=True, timeout=20)
        return (p.stdout+p.stderr).decode('latin1')

    def find(directory, name):
        return next(p for p in directory.iterdir() if p.name.upper() == name)

    (wd / 'ONE.TXT').write_bytes(b'ONE\r\n')
    (wd / 'TWO.TXT').write_bytes(b'TWO\r\n')
    (wd / 'THREE.TXT').write_bytes(b'THREE\r\n')
    run('arc86', 'a', 'BASE', 'ONE.TXT')
    default = find(wd, 'BASE.ARC')
    original = default.read_bytes()
    (drive / 'BASE.ARC').write_bytes(original)
    (drive / 'FS0000.TMP').write_bytes(b'EXISTING TEMP')
    run('arc86', 'ab', 'B:BASE', 'TWO.TXT')
    assert default.read_bytes() == original
    assert find(drive, 'BASE.BAK').read_bytes() == original
    listing = run('arc86', 'v', 'B:BASE').upper()
    assert 'ONE.TXT' in listing and 'TWO.TXT' in listing, listing
    run('arc86', 'a', 'SRC', 'THREE.TXT')
    run('marc86', 'B:BASE.ARC', 'SRC.ARC')
    assert default.read_bytes() == original
    listing = run('arc86', 'v', 'B:BASE').upper()
    assert all(name in listing for name in ['ONE.TXT', 'TWO.TXT', 'THREE.TXT']), listing
    assert 'No errors detected' in run('arc86', 't', 'B:BASE')
    assert (drive / 'FS0000.TMP').read_bytes() == b'EXISTING TEMP'
print('ARC/MARC: cross-drive replacement, backup, collision and archive integrity PASS')
