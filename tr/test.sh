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

check([r'\141-\172', '%A-%Z', 'IN.TXT'], 'abc xyz\n', 'ABC XYZ\n')
check(['-D', r'\170', 'z', 'IN.TXT'], 'axbxx\n', 'ab\n')
check(['-S', r'\170', r'\170', 'IN.TXT'], 'axxxbxx\n', 'axbx\n')
check(['-CD', r'\141', 'z', 'IN.TXT'], 'abcaa\n', 'aaa')
check(['-O', 'OUT.TXT', '-P', r'\141', r'\142', 'IN.TXT'], 'a\n'*30, 'b\n'*30, output=True)
check(['-P', '-o', 'OUT.TXT', r'\141', r'\142', 'IN.TXT'], 'a\n'*30, 'b\n'*30, output=True)
check(['-o'], '', diagnostic='usage: tr')
check(['-Z'], '', diagnostic='usage: tr')
check([r'\141', r'\142', 'MISSING.TXT'], '', diagnostic='cannot open')

check(['A-Z', '%A-%Z', 'IN.TXT'], 'abc xyz\n', 'ABC XYZ\n')
check(['%A-%Z', 'A-Z', 'IN.TXT'], 'ABC XYZ\n', 'abc xyz\n')
check(['%A', 'Z', 'IN.TXT'], 'Aa\n', 'za\n')
check([r'\%', 'X', 'IN.TXT'], '100%\n', '100x\n')
EOL_CASES = [(b'a\na\nb\n', b'b\r\nb\r\nb\r\n'), (b'a\r\na\r\nb\r\n', b'b\r\nb\r\nb\r\n'), (b'a\na\nb', b'b\r\nb\r\nb'), (b'a\r\na\r\nb\x1aignored\r\n', b'b\r\nb\r\nb'), (b'\x1aignored', b''), (b'', b'')]
EOL_ARGS = ['-o','OUT.TXT','a','b','IN.TXT']
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
