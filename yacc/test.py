"""Run embedded-template Yacc and its generated parser under CP/M-86."""
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
CC = shlex.split(os.environ.get('YACC_TEST_CC', 'aztec43_acc'))
CMDMOD = shlex.split(os.environ.get('YACC_TEST_CMDMOD', 'cmdmod'))
MAKE = shlex.split(os.environ.get('YACC_TEST_MAKE', 'make'))


def run(command, wd):
    p = subprocess.run(command, cwd=wd, stdin=subprocess.DEVNULL,
                       capture_output=True, start_new_session=True, timeout=30)
    output = (p.stdout+p.stderr).decode('latin1')
    assert p.returncode == 0, (command, output)
    return output


def group(image):
    assert [image[i*9] for i in range(4)] == [1, 2, 3, 0]
    offset = 128 + sum(struct.unpack_from('<H', image, i*9+1)[0]*16
                       for i in range(2))
    size = struct.unpack_from('<H', image, 19)[0]*16
    assert struct.unpack_from('<HH', image, 23) == (size//16, size//16)
    return offset, size


def generate(binary, wd, grammar=None):
    # Each invocation gets a directory containing only the CMD and grammar.
    (wd/'YACC.CMD').write_bytes(binary)
    (wd/'CALC.Y').write_bytes(grammar or (ROOT/'testcalc.y').read_bytes())
    output = run(['emu2', 'YACC.CMD', 'CALC.Y'], wd)
    assert 'error' not in output.lower(), output
    generated = (wd/'ytab.c').read_bytes()
    assert b'yyparse()' in generated and b'$A' not in generated
    assert b'\0' not in generated and b'\xff' not in generated
    return generated


def exercise_parser(wd):
    run(CC + ['-target=cpm86', '-o', 'parser.cmd', 'ytab.c'], wd)
    for expression, expected in [('7+5-2', 'RESULT 10'),
                                 ('2+3*4', 'RESULT 14'),
                                 ('(7+5)*2', 'RESULT 24'),
                                 ('20-3-2', 'RESULT 15'), ('1+', 'REJECT')]:
        output = run(['emu2', 'parser.cmd', expression], wd)
        assert output.strip() == expected, (expression, output)


image = (ROOT/'yacc.cmd').read_bytes()
template = (ROOT/'yaccpar.tpl').read_bytes()
offset, size = group(image)
assert image[offset:offset+6] == b'YTPL'+struct.pack('<H', len(template))
assert image[offset+6:offset+6+len(template)] == template

with tempfile.TemporaryDirectory() as folder:
    base = Path(folder)
    normal = base/'normal'; normal.mkdir()
    expected = generate(image, normal)
    exercise_parser(normal)

    # NUL-filled records and Ctrl-Z terminated grammar tails both stop cleanly.
    marked = base/'marked'; marked.mkdir()
    text = (ROOT/'testcalc.y').read_bytes().replace(b'\n', b'\r\n')
    generate(image, marked, text+b'\x1aTHIS MUST NOT ENTER THE OUTPUT')
    exercise_parser(marked)

    # Poison paragraph padding to prove the exact payload length is respected.
    padded = base/'padded'; padded.mkdir()
    variant = bytearray(image)
    end = offset+6+len(template)
    variant[end:offset+size] = b'\xff'*(offset+size-end)
    assert generate(variant, padded) == expected

    for case in ('missing', 'magic', 'empty', 'length'):
        wd = base/case; wd.mkdir()
        binary = bytearray(image)
        if case == 'magic': binary[offset] = 0
        if case == 'empty': binary[offset+4:offset+6] = b'\0\0'
        if case == 'length': binary[offset+4:offset+6] = b'\xff\xff'
        (wd/'YACC.CMD').write_bytes(binary)
        if case == 'missing':
            run(CMDMOD + ['-d', 'YACC.CMD', '2'], wd)
        (wd/'CALC.Y').write_bytes((ROOT/'testcalc.y').read_bytes())
        p = subprocess.run(['emu2', 'YACC.CMD', 'CALC.Y'], cwd=wd,
                           stdin=subprocess.DEVNULL, capture_output=True,
                           start_new_session=True, timeout=10)
        output = (p.stdout+p.stderr).decode('latin1')
        # CP/M exit status is not portable; require the diagnostic and no output.
        assert 'missing or invalid embedded parser template' in output, output
        assert not (wd/'ytab.c').exists(), case

    # Exercise the actual build dependency in a private copy of the project.
    build = base/'build'; build.mkdir()
    for path in ROOT.iterdir():
        if path.is_file() and (path.suffix in ('.c', '.h', '.o', '.cmd', '.bin', '.tpl')
                               or path.name in ('Makefile', 'packtpl.py')):
            shutil.copy2(path, build/path.name)
    marker = b'\r\n/* EMBEDDED REVISION $Z */'
    (build/'yaccpar.tpl').write_bytes(template+marker)
    # -W makes this deterministic even on filesystems with coarse timestamps.
    run(MAKE + ['-W', 'yaccpar.tpl', 'yacc.cmd',
                'CC='+shlex.join(CC), 'LD='+shlex.join(CC),
                'CMDMOD='+shlex.join(CMDMOD)], build)
    changed = base/'changed'; changed.mkdir()
    generated = generate((build/'yacc.cmd').read_bytes(), changed)
    assert marker in generated
    exercise_parser(changed)

print('Yacc: isolated CMD generation, parser execution, CR/LF and EOF, bounded payload,')
print('      missing/invalid groups and template rebuild PASS')
