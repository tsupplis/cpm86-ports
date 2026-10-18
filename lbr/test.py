"""Small LBR binary round-trip and update regression suite."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
checks=0

def run(wd, binary, *args):
    p=subprocess.run(['emu2',str(root/(binary+'.cmd')),*args],cwd=wd,
                     stdin=subprocess.DEVNULL,capture_output=True,start_new_session=True,timeout=5)
    return (p.stdout+p.stderr).decode('latin1')

def file(wd,name):
    return next(p for p in wd.iterdir() if p.name.upper()==name.upper())

for size in (0,127,128,256,4096):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        payload=bytes((i*37+26)%256 for i in range(size))
        (wd/'DATA.BIN').write_bytes(payload)
        run(wd,'lu86','un16','TEST','DATA.BIN')
        file(wd,'DATA.BIN').unlink()
        run(wd,'lu86','e','TEST')
        # The LBR format stores CP/M records, not the original byte count.
        expected=payload+b'\0'*((-size)%128)
        assert file(wd,'DATA.BIN').read_bytes()==expected,('round trip',size)
        checks+=1
        file(wd,'DATA.BIN').unlink()
        run(wd,'delbr','TEST')
        assert file(wd,'DATA.BIN').read_bytes()==expected,('delbr round trip',size)
        checks+=1

with tempfile.TemporaryDirectory() as folder:
    wd=Path(folder);a=b'A'*128;b=bytes(range(256));updated=b'Z'*256
    (wd/'A.BIN').write_bytes(a);(wd/'B.BIN').write_bytes(b)
    run(wd,'lu86','un16','TEST','A.BIN','B.BIN')
    (wd/'A.BIN').write_bytes(updated)
    run(wd,'lu86','u','TEST','A.BIN')
    run(wd,'lu86','d','TEST','B.BIN')
    run(wd,'lu86','r','TEST')
    listing=run(wd,'lu86','t','TEST').upper()
    assert 'A.BIN' in listing and 'B.BIN' not in listing,listing
    file(wd,'A.BIN').unlink();file(wd,'B.BIN').unlink()
    run(wd,'lu86','e','TEST')
    assert file(wd,'A.BIN').read_bytes()==updated
    assert not any(p.name.upper()=='B.BIN' for p in wd.iterdir())
    checks+=2
    archive=file(wd,'TEST.LBR')
    archive.write_bytes(archive.read_bytes()[:128])
    text=run(wd,'lu86','t','TEST').lower()
    assert "can't read directory" in text,text
    checks+=1
print('LBR: %d binary round-trip, update/delete/reorganize, and short-directory checks passed'%checks)
