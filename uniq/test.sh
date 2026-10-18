#!/bin/sh
cd "$(dirname "$0")" || exit 1
python3 - <<'PYTEST'
import pathlib, subprocess, tempfile, random
root = pathlib.Path.cwd()
count = 0
def check(args, data, expected=None, output=False, diagnostic=None, extra=None):
    global count
    for ext in ('cmd', 'com'):
        with tempfile.TemporaryDirectory() as folder:
            wd = pathlib.Path(folder)
            (wd/'IN.TXT').write_text(data)
            for name, contents in (extra or {}).items(): (wd/name).write_text(contents)
            p = subprocess.run(['emu2', str(root/(root.name+'.'+ext)), *args],
                               cwd=wd, capture_output=True, timeout=20)
            text = p.stdout.decode('latin1').replace('\r', '')
            if diagnostic:
                assert diagnostic in text + p.stderr.decode('latin1'), (ext,args,text,p.stderr)
                if ext == 'com': assert p.returncode != 0, (args,p.returncode)
            else:
                assert p.returncode == 0, (ext,args,p.returncode,p.stderr)
                if output:
                    text = (wd/'OUT.TXT').read_bytes().decode('latin1').replace('\r','').split('\x1a')[0]
                elif ext == 'cmd' and text.endswith('\n'):
                    text = text[:-1] # BDOS termination emits a console newline
                assert text == expected, (ext,args,repr(text[:300]),repr(expected[:300]))
            assert not list(wd.glob('SRT*.TMP')), (ext,args,'temporary files left behind')
            count += 1

check(['IN.TXT'], 'a\na\nb\nb', 'a\nb\n')
check(['-C', 'IN.TXT'], 'a\na\nb\n', '   2 a\n   1 b\n')
check(['-D', 'IN.TXT'], 'a\na\nb\n', 'a\n')
check(['-u', 'IN.TXT'], 'a\na\nb\n', 'b\n')
check(['-1', 'IN.TXT'], 'one x\ntwo x\n', 'one x\n')
check(['+1', 'IN.TXT'], 'ax\nbx\n', 'ax\n')
check(['-O','OUT.TXT','-P','IN.TXT'], ''.join('%s\n'%i for i in range(30)), ''.join('%s\n'%i for i in range(30)), output=True)
check(['IN.TXT','OUT.TXT'], 'a\na\n', 'a\n', output=True)
check(['-o'], '', diagnostic='usage: uniq')
check(['-Z'], '', diagnostic='usage: uniq')
check(['IN.TXT'], 'a'*1000, diagnostic='line too long')

EOL_CASES = [(b'a\na\nb\n', b'a\r\nb\r\n'), (b'a\r\na\r\nb\r\n', b'a\r\nb\r\n'), (b'a\na\nb', b'a\r\nb\r\n'), (b'a\r\na\r\nb\x1aignored\r\n', b'a\r\nb\r\n'), (b'\x1aignored', b''), (b'', b'')]
EOL_ARGS = ['-o','OUT.TXT','IN.TXT']
# Verify actual output bytes, without removing CR or normalizing newlines.
for ext in ('cmd', 'com'):
    for data, expected in EOL_CASES:
        with tempfile.TemporaryDirectory() as folder:
            wd = pathlib.Path(folder)
            (wd/'IN.TXT').write_bytes(data)
            args = EOL_ARGS
            p = subprocess.run(['emu2', str(root/(root.name+'.'+ext)), *args],
                               cwd=wd, capture_output=True, timeout=20)
            raw = (wd/'OUT.TXT').read_bytes()
            assert p.returncode == 0, (ext, p.stderr)
            if ext == 'cmd':
                payload, marker, padding = raw.partition(b'\x1a')
                assert (not raw or marker) and len(raw) % 128 == 0, (ext,raw)
            else:
                payload = raw
            assert payload == expected, (ext, data, raw)
            count += 1
print('%d target checks passed' % count)
PYTEST
