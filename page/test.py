"""Production file tests plus deterministic console-shim paging tests."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent

with tempfile.TemporaryDirectory(prefix='page-test-') as tmp:
    work = Path(tmp)
    # Only keyboard input/status are stubbed. File operations and output use
    # real emulated BDOS. This private diagnostic build is never distributed.
    source = (ROOT/'page.a86').read_text().replace('INT     224', 'CALL    TESTBDOS')
    shim = '''TESTBDOS:
        CMP CL,6
        JZ TESTKEY
        CMP CL,11
        JZ TESTSTATUS
        INT 224
        RET
TESTKEY:
        MOV AL,' '
        RET
TESTSTATUS:
        XOR AL,AL
        RET
'''
    source = source.replace('L_7     EQU', shim+'\nL_7     EQU')
    (work/'page.a86').write_bytes(source.replace('\n','\r\n').encode())
    shutil.copy(ROOT/'Makefile', work)
    build = subprocess.run(['make'], cwd=work, capture_output=True)
    assert build.returncode == 0, build.stdout+build.stderr

    def run(data, version, missing=False, keyboard=False):
        path = work/'input.txt'
        if missing:
            path.unlink(missing_ok=True)
        else:
            path.write_bytes(data)
        env = dict(os.environ, EMU2_DRIVE_C=tmp, EMU2_DEFAULT_DRIVE='C:',
                   EMU2_CPM_VER=version)
        binary = (work if keyboard else ROOT)/'page.cmd'
        result = subprocess.run([os.environ.get('EMU2','emu2'), str(binary),
                                 'INPUT.TXT'], cwd=work, env=env, input=b'',
                                capture_output=True, timeout=10)
        assert result.returncode == 0, result.stdout+result.stderr
        assert b'\x1b' not in result.stdout, result.stdout
        return result.stdout

    for version in ('2.2','3.1'):
        for ending in (b'\r\n',b'\r',b'\n'):
            out = run(ending.join([b'LINE']*26)+b'\x1a', version, keyboard=True)
            assert out.count(b'LINE') == 26 and out.count(b'[more]') == 1, out
            assert b'LINE\r\nLINE' in out, out
        out = run(b'A'*256, version)
        assert out.count(b'A'*79+b'\r\n') == 3, out
        assert b'File Read Error' not in out and b'End of Input' in out, out
        out = run(b'\x1aHIDDEN', version)
        assert b'HIDDEN' not in out and b'End of Input' in out, out
        out = run(b'', version)
        assert b'End of Input' in out and b'File Read Error' not in out, out
        out = run(b'', version, missing=True)
        assert b'File Open Error' in out, out
        out = run(b'A\tB\rC\tD\x1a', version)
        assert b'A       B\r\nC       D' in out, out
        print(f'PASS: {version}: 5 production cases, 3 console-shim paging cases')
