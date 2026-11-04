"""Z80 loader size boundaries using a small 8080 program."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
# Print through BDOS, then jump to the emulated warm boot.
program=bytes.fromhex('0e 09 11 0b 01 cd 05 00 c3 00 00')+b'LOADER-OK$'
limit=0xbf00-0x100
checks=0
for size in (len(program),limit-128,limit,limit+1,65536):
    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        (wd/'PROBE.COM').write_bytes(program+b'\0'*(size-len(program)))
        p=subprocess.run(['emu2',str(root/'z80.cmd'),'PROBE.COM'],cwd=wd,
                         stdin=subprocess.DEVNULL,capture_output=True,timeout=10)
        text=(p.stdout+p.stderr).decode('latin1')
        if size<=limit:
            assert 'LOADER-OK' in text and 'exceeds TPA' not in text,(size,text)
        else:
            assert 'exceeds TPA' in text and 'LOADER-OK' not in text,(size,text)
        assert 'Branch outside' not in text,(size,text)
        checks+=1
print('Z80: %d normal, exact-limit and oversized loader checks passed'%checks)
