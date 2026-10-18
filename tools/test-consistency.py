#!/usr/bin/env python3
"""Cross-port checks. Run all tools, or name tools to check.

    python3 tools/test-consistency.py
    python3 tools/test-consistency.py grep sed

Build the selected tools first. Requires emu2 and both .cmd/.com binaries.
"""
import argparse
import os
import pathlib
import select
import subprocess
import tempfile
import time

TOOLS = ('diff', 'grep', 'sed', 'tr', 'uniq', 'sort')
REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent


def check_tool(tool):
    root = REPO_ROOT / tool
    checks = 0

    def run(ext, args, files, expected, status=0):
        nonlocal checks
        with tempfile.TemporaryDirectory() as folder:
            wd = pathlib.Path(folder)
            for name, data in files.items():
                (wd/name).write_bytes(data)
            proc = subprocess.run(['emu2', str(root/(tool+'.'+ext)), *args],
                                  cwd=wd, stdin=subprocess.DEVNULL, start_new_session=True,
                                  capture_output=True, timeout=10)
            if ext == 'com':
                assert proc.returncode == status, (tool, ext, args, proc.returncode, proc.stdout, proc.stderr)
            raw = (wd/'OUT.TXT').read_bytes()
            if ext == 'cmd':
                payload, marker, padding = raw.partition(b'\x1a')
                assert (not raw or marker) and len(raw) % 128 == 0, (tool, raw)
            else:
                payload = raw
            assert payload == expected, (tool, ext, args, files, raw, expected)
            checks += 1

    def operands():
        return {'tr':['a','b','IN.TXT'], 'uniq':['IN.TXT'],
                'sort':['IN.TXT'], 'grep':['.','IN.TXT'],
                'sed':['S/A/B/G','IN.TXT'], 'diff':['IN.TXT','NEW.TXT']}[tool]

    for ext in ('cmd','com'):
        for data in (b'a\na\n', b'a\r\na\r\n', b'a\na', b'a\na\x1aignored\r\n', b''):
            files={'IN.TXT':data, 'NEW.TXT':b'a\r\na\r\n' if data else b''}
            result=(b'b\r\nb\r\n' if tool in ('tr','sed') else b'a\r\n' if tool == 'uniq' else b'a\r\na\r\n') if data else b''
            if tool == 'diff': result=b''
            if tool == 'tr' and data and not data.endswith(b'\n'):
                result=result[:-2]
            if tool == 'tr' and b'\x1a' in data:
                result=b'b\r\nb'
            run(ext, ['-o','OUT.TXT',*operands()], files, result, 1 if tool=='grep' and not data else 0)
        # More than a screenful proves -o wins in either argument order.
        data=b'a\n'*30
        files={'IN.TXT':data,'NEW.TXT':b'b\n'*30}
        if tool in ('tr','sed'): result=b'b\r\n'*30
        elif tool == 'uniq':
            files['IN.TXT']=b''.join(('%02d\n'%i).encode() for i in range(30))
            result=files['IN.TXT'].replace(b'\n',b'\r\n')
        elif tool == 'diff': result=b'1,30c1,30\r\n'+b'< a\r\n'*30+b'---\r\n'+b'> b\r\n'*30
        else: result=b'a\r\n'*30
        for upper in (False,True):
            o,p=('-O','-P') if upper else ('-o','-p')
            for flags in ([o,'OUT.TXT',p],[p,o,'OUT.TXT']):
                run(ext,[*flags,*operands()],files,result,1 if tool=='diff' else 0)
        if tool == 'diff':
            for old,new in [(b'a\r\nb',b'a\nb\r\n'), (b'a\nb\x1aignored\n',b'a\r\nb\r\n')]:
                run(ext,['-o','OUT.TXT','IN.TXT','NEW.TXT'],{'IN.TXT':old,'NEW.TXT':new},b'')
            run(ext,['-o','OUT.TXT','IN.TXT','NEW.TXT'],{'IN.TXT':b'a\r\nx','NEW.TXT':b'a\ny'},b'2c2\r\n< x\r\n---\r\n> y\r\n',1)
        if tool == 'sed':
            for script in (b's/a/b/g\r\n',b's/a/b/g',b's/a/b/g\x1ainvalid\n'):
                run(ext,['-f','SCRIPT.TXT','-o','OUT.TXT','IN.TXT'],{'SCRIPT.TXT':script,'IN.TXT':b'a\r\na'},b'b\r\nb\r\n')
            run(ext,['-n','-o','OUT.TXT','$P','IN.TXT'],{'IN.TXT':b'a\r\nb'},b'b\r\n')
            run(ext,['-o','OUT.TXT','P','IN.TXT','NEW.TXT'],{'IN.TXT':b'a\x1aignored\n','NEW.TXT':b'b\n'},b'a\r\na\r\nb\r\nb\r\n')
        if tool == 'grep':
            run(ext,['-h','-o','OUT.TXT','.','IN.TXT','NEW.TXT'],{'IN.TXT':b'a\x1aignored\n','NEW.TXT':b'b'},b'a\r\nb\r\n')

    if tool == 'diff':
        for ext in ('cmd','com'):
            old = ['line%03d %s'%(i,'x'*30) for i in range(300)]
            new = old[:]; new[130]='replacement'
            expected=('131c131\r\n< '+old[130]+'\r\n---\r\n> replacement\r\n').encode()
            run(ext,['-o','OUT.TXT','IN.TXT','NEW.TXT'],
                {'IN.TXT':('\r\n'.join(old)).encode(),'NEW.TXT':('\n'.join(new)+'\n').encode()},expected,1)
            for flags in (['-D','%NAME'],['-d%NAME']):
                run(ext,[*flags,'-o','OUT.TXT','IN.TXT','NEW.TXT'],
                    {'IN.TXT':b'a\r\nx','NEW.TXT':b'a\ny'},
                    b'a\r\n#ifndef NAME\r\nx\r\n#else NAME\r\ny\r\n#endif NAME\r\n',1)
            run(ext,['-o','OUT.TXT','-D'],{},b'',2)

    if tool == 'sed':
        for ext in ('cmd','com'):
            run(ext,['-n','-f','SCRIPT.TXT','-o','OUT.TXT','IN.TXT'],
                {'SCRIPT.TXT':b'N\np\n','IN.TXT':b'a\r\nb'},b'a\r\nb\r\n')
            run(ext,['-f','SCRIPT.TXT','-o','OUT.TXT','IN.TXT'],
                {'SCRIPT.TXT':b'r EXTRA.TXT\r\n','IN.TXT':b'a\r\n','EXTRA.TXT':b'b\r\nc\x1aignored'},b'a\r\nb\r\nc')
            for flag in ('-f','-e'):
                run(ext,['-o','OUT.TXT',flag],{},b'',2)

    # Read until the prompt, then stop the emulator: no synthetic key is assumed.
    # This checks physical newline counts, including less common output modes.
    for ext in ('cmd','com'):
        variants=[[]]
        if tool == 'diff': variants += [['-c'],['-e'],['-D%NAME']]
        if tool == 'sed': variants += [['-n','-e','%N','-e','P']]
        for flags in variants:
            with tempfile.TemporaryDirectory() as folder:
                wd=pathlib.Path(folder)
                data=b''.join(('a%02d\n'%i).encode() for i in range(30))
                (wd/'IN.TXT').write_bytes(data)
                (wd/'NEW.TXT').write_bytes(data.replace(b'a',b'b'))
                args=['-p',*flags,*operands()]
                if tool == 'sed' and flags: args=['-p',*flags,'IN.TXT']
                proc=subprocess.Popen(['emu2',str(root/(tool+'.'+ext)),*args],cwd=wd,
                                      stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                captured=b''
                deadline=time.monotonic()+5
                try:
                    while b'[more]' not in captured and time.monotonic()<deadline:
                        ready,_,_=select.select([proc.stdout],[],[],0.1)
                        if ready:
                            block=os.read(proc.stdout.fileno(),65536)
                            if not block: break
                            captured+=block
                    before,marker,after=captured.partition(b'[more]')
                    assert marker and before.count(b'\n')==23, (tool,ext,flags,captured)
                    checks+=1
                finally:
                    if proc.poll() is None: proc.terminate()
                    try:
                        proc.communicate(timeout=2)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.communicate()
    print('%s: %d consistency checks passed' % (tool,checks))

    return checks


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('tools', nargs='*', metavar='TOOL',
                        help='tools to check (default: all six)')
    args = parser.parse_args()
    unknown = sorted(set(args.tools) - set(TOOLS))
    if unknown:
        parser.error('unknown tool(s): %s; choose from %s' %
                     (', '.join(unknown), ', '.join(TOOLS)))
    total = sum(check_tool(tool) for tool in (args.tools or TOOLS))
    print('%d consistency checks passed in total' % total)


if __name__ == '__main__':
    main()
