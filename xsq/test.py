"""Small squeeze/unsqueeze binary integrity and checksum regression suite."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
checks=0

def run(wd,binary,*args):
    p=subprocess.run(['emu2',str(root/(binary+'.cmd')),*args],cwd=wd,
                     stdin=subprocess.DEVNULL,capture_output=True,start_new_session=True,timeout=5)
    return (p.stdout+p.stderr).decode('latin1')

def file(wd,name):
    return next(p for p in wd.iterdir() if p.name.upper()==name.upper())

for payload in (b'',b'X'*127,b'\0'*128,bytes(range(256)),b'\x90'*4096,bytes(range(256))*16):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder);(wd/'DATA.BIN').write_bytes(payload)
        run(wd,'sq','DATA.BIN')
        file(wd,'DATA.BIN').unlink()
        text=run(wd,'usq','-U','DATA.BQN')
        assert 'bad checksum' not in text.lower(),text
        expected=payload+b'\0'*((-len(payload))%128)
        assert file(wd,'DATA.BIN').read_bytes()==expected,len(payload)
        checks+=1

with tempfile.TemporaryDirectory() as folder:
    wd=Path(folder);(wd/'DATA.BIN').write_bytes(bytes(range(256)))
    run(wd,'sq','DATA.BIN')
    archive=file(wd,'DATA.BQN');data=bytearray(archive.read_bytes());data[2]^=1
    archive.write_bytes(data)
    assert 'bad checksum' in run(wd,'usq','-U','DATA.BQN').lower()
    (wd/'BAD.SQ').write_bytes(b'not a squeezed file')
    assert 'not a squeezed file' in run(wd,'usq','BAD.SQ').lower()
    checks+=2
print('XSQ: %d binary round-trip and invalid/checksum checks passed'%checks)
