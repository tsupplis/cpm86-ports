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
                    assert (wd/'OUT.TXT').exists(), (ext,args,text,p.stderr)
                    text = (wd/'OUT.TXT').read_bytes().decode('latin1').replace('\r','').split('\x1a')[0]
                elif ext == 'cmd' and text.endswith('\n'):
                    text = text[:-1] # BDOS termination emits a console newline
                assert text == expected, (ext,args,repr(text[:300]),repr(expected[:300]))
            for name, contents in (extra or {}).items():
                if name.startswith('SRT'):
                    assert (wd/name).read_text() == contents, (ext,args,'existing temp changed')
            assert {p.name for p in wd.glob('SRT*.TMP')} == {n for n in (extra or {}) if n.startswith('SRT')}, (ext,args,'temporary files left behind')
            count += 1

check(['IN.TXT'], 'z\na\nb', 'a\nb\nz\n')
check(['-R','IN.TXT'], 'z\na\nb\n', 'z\nb\na\n')
check(['-N','IN.TXT'], '10\n2\n-3\n', '-3\n2\n10\n')
check(['-U','IN.TXT'], 'b\na\nb\n', 'a\nb\n')
check(['-F','IN.TXT'], 'b\nA\nc\n', 'A\nb\nc\n')
check(['+1N','IN.TXT'], 'x 10\ny 2\n', 'y 2\nx 10\n')
check(['-T:', '+1N', 'IN.TXT'], 'x:10\ny:2\n', 'y:2\nx:10\n')
check(['-M','-O','OUT.TXT','IN.TXT','OUT.TXT'], 'a\nc', 'a\nb\nc\nd\n', output=True, extra={'OUT.TXT':'b\nd\n'})
check(['-C','IN.TXT'], 'a\nb\n', '')
check(['-C','IN.TXT'], 'b\na\n', diagnostic='disorder:')
check(['-CU','IN.TXT'], 'a\na\n', diagnostic='nonunique:')
check(['-O','OUT.TXT','-P','IN.TXT'], ''.join('%04d\n'%i for i in range(40,0,-1)), ''.join('%04d\n'%i for i in range(1,41)), output=True)
check(['-o'], '', diagnostic='usage: sort')
check(['-Z'], '', diagnostic='usage: sort')
check(['IN.TXT'], 'a'*1023, diagnostic='line too long')
rng=random.Random(17)
lines=['%04d %s\n'%(rng.randrange(1000),'x'*85) for _ in range(3500)]
check(['-O','OUT.TXT','IN.TXT'], ''.join(lines), ''.join(sorted(lines)), output=True)
check(['-U','-O','OUT.TXT','IN.TXT'], ''.join(lines), ''.join(sorted(set(lines))), output=True)


# Separator case follows sed, independently of option-letter folding.
check(['-TA', '+1N', 'IN.TXT'], 'xa10\nya2\n', 'ya2\nxa10\n')
check(['-t%A', '+1N', 'IN.TXT'], 'xA10\nyA2\n', 'yA2\nxA10\n')
check([r'-t\%', '+1N', 'IN.TXT'], 'x%10\ny%2\n', 'y%2\nx%10\n')
check(['-t'], '', diagnostic='usage: sort')
check(['-t%'], '', diagnostic='usage: sort')
check(['-P','-O','OUT.TXT','IN.TXT'], 'b\na\n'*30, 'a\n'*30+'b\n'*30, output=True)
# Errors after a run exists must remove only the files created by this run.
check(['-O','OUT.TXT','IN.TXT'], ''.join(lines), diagnostic='temporary file already exists', extra={'SRT00AA.TMP':'keep me\n'})
check(['-O','OUT.TXT','IN.TXT'], ''.join(lines), diagnostic='temporary file already exists', extra={'SRT00AB.TMP':'keep me too\n'})
check(['-O','SRT00AA.TMP','IN.TXT'], ''.join(lines), diagnostic='output conflicts with temporary file')
check(['IN.TXT','MISSING.TXT'], ''.join(lines), diagnostic='cannot open')

# Inspect raw output: CRLF, final unterminated lines, Ctrl-Z, and empty files.
for ext in ('cmd','com'):
    for merge in (False, True):
        for data, expected in [
            (b'a\nb\n', b'a\r\nb\r\n'),
            (b'a\r\nb\r\n', b'a\r\nb\r\n'),
            (b'a\nb', b'a\r\nb\r\n'),
            (b'a\nb\x1aignored\n', b'a\r\nb\r\n'),
            (b'a\n\x1aignored\n', b'a\r\n'),
            (b'\x1aignored\n', b''),
            (b'', b'')]:
            with tempfile.TemporaryDirectory() as folder:
                wd=pathlib.Path(folder)
                (wd/'IN.TXT').write_bytes(data)
                args=(['-M'] if merge else [])+['-O','OUT.TXT','IN.TXT']
                p=subprocess.run(['emu2',str(root/('sort.'+ext)),*args],cwd=wd,capture_output=True,timeout=20)
                assert p.returncode == 0, (ext,args,p.stderr)
                raw=(wd/'OUT.TXT').read_bytes()
                if ext == 'cmd':
                    payload,marker,padding=raw.partition(b'\x1a')
                    assert (not raw or marker) and len(raw)%128 == 0, (ext,raw)
                else: payload=raw
                assert payload == expected, (ext,merge,data,raw)
                assert not list(wd.glob('SRT*.TMP'))
                count+=1
# Check mode must not truncate its input or an unrelated output destination.
for ext in ('cmd','com'):
    for output in ('IN.TXT','OUT.TXT'):
        with tempfile.TemporaryDirectory() as folder:
            wd=pathlib.Path(folder)
            original=b'a\r\nb\r\n'
            (wd/'IN.TXT').write_bytes(original)
            (wd/'OUT.TXT').write_bytes(b'keep me')
            p=subprocess.run(['emu2',str(root/('sort.'+ext)),'-C','-O',output,'IN.TXT'],cwd=wd,capture_output=True,timeout=20)
            assert p.returncode == 0, (ext,output,p.stderr)
            assert (wd/'IN.TXT').read_bytes() == original
            assert (wd/'OUT.TXT').read_bytes() == b'keep me'
            count+=1
print('%d target checks passed' % count)
PYTEST
