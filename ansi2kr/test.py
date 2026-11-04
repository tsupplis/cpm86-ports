"""Source-preservation regressions for host and CP/M-86 ANSI2KR."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parent
source=b'int main(void)\n{\n  return 7;\n}\n'
checks=0

for target in ('host','cpm86'):
    command=[str(root/'ansi2kr')] if target=='host' else ['emu2',str(root/'ansi2kr.cmd')]
    aliases=[('SRC.C','SRC.C')]
    if target=='host':
        aliases += [('SRC.C','./SRC.C'),('SRC.C','HARD.C'),('SRC.C','LINK.C')]
    else:
        aliases += [('SRC.C','src.c'),('A:SRC.C','a:src.c'),('PLAIN','PLAIN.')]
    for infile,outfile in aliases:
        with tempfile.TemporaryDirectory() as folder:
            wd=Path(folder)
            name='PLAIN' if infile=='PLAIN' else 'SRC.C'
            original=wd/name
            original.write_bytes(source)
            if outfile=='HARD.C': os.link(original,wd/outfile)
            if outfile=='LINK.C': (wd/outfile).symlink_to(original.name)
            p=subprocess.run(command+[infile,outfile],cwd=wd,
                             stdin=subprocess.DEVNULL,capture_output=True,timeout=5)
            text=(p.stdout+p.stderr).decode('latin1')
            assert 'same file' in text,(target,infile,outfile,text)
            assert original.read_bytes()==source,(target,infile,outfile,text)
            checks+=1

    with tempfile.TemporaryDirectory() as folder:
        wd=Path(folder)
        (wd/'SRC.C').write_bytes(source)
        p=subprocess.run(command+['SRC.C','OUT.C'],cwd=wd,
                         stdin=subprocess.DEVNULL,capture_output=True,timeout=5)
        text=(p.stdout+p.stderr).decode('latin1')
        assert p.returncode==0,text
        assert (wd/'SRC.C').read_bytes()==source,text
        output=next(p for p in wd.iterdir() if p.name.upper()=='OUT.C').read_bytes()
        assert b'main()' in output and b'return 7;' in output,(target,output,text)
        checks+=1
print('ANSI2KR: %d same-file preservation and normal-conversion checks passed'%checks)
