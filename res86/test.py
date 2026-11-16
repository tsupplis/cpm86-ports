"""Exercise the production CMD interactively; private files, bounded prompt waits.

PTY input/output avoids stdio buffering and EOF busy loops. Every command must
return to its real prompt; saved records are checked independently on the host.
"""
import os
from pathlib import Path
import pty
import re
import select
import struct
import subprocess
import tempfile
import time
import tty

ROOT = Path(__file__).resolve().parent


class Session:
    def __init__(self, directory, version, binary):
        self.directory = directory
        self.master, slave = pty.openpty()
        tty.setraw(slave)
        env = dict(os.environ, EMU2_CPM_VER=version,
                   EMU2_DRIVE_C=str(directory), EMU2_DEFAULT_DRIVE='C:')
        self.process = subprocess.Popen(
            [os.environ.get('EMU2', 'emu2'), '-c', '8086', str(binary)],
            cwd=directory, env=env, stdin=slave, stdout=slave, stderr=slave)
        os.close(slave)
        assert '8086 Ver. 2.4' in self.read()

    def read(self):
        output = bytearray()
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if select.select([self.master], [], [], .05)[0]:
                output.extend(os.read(self.master, 65536))
                assert len(output) < 1000000, 'Unbounded console output'
                if re.search(rb'[CDES]->$', output):
                    return output.decode('ascii', errors='replace')
            assert self.process.poll() is None, bytes(output[-2000:])
        raise AssertionError('Prompt timeout: ' + repr(output[-2000:]))

    def command(self, command):
        os.write(self.master, command.encode('ascii') + b'\r')
        return self.read()

    def close(self):
        self.process.terminate()
        self.process.wait(timeout=5)
        os.close(self.master)


def image(groups, body, trailer=b''):
    header = bytearray(128)
    for index, (kind, paragraphs) in enumerate(groups):
        struct.pack_into('<BHHHH', header, index * 9,
                         kind, paragraphs, 0, paragraphs, 0)
    body += bytes((-len(body)) % 128)
    return bytes(header) + body + trailer


def suite(version, binary):
    count = 0
    with tempfile.TemporaryDirectory(prefix='res86-test-') as directory:
        work = Path(directory)
        session = Session(work, version, binary)
        def check(name, action):
            nonlocal count
            try:
                action()
            except Exception as error:
                raise AssertionError(name) from error
            count += 1
        def contains(command, text):
            out = session.command(command)
            assert text in out, out
        def file(name, data):
            (work / name).write_bytes(data)
        def roundtrip(suffix, data):
            file('input.' + suffix, data + b'\x1a')
            out = session.command('rinput.' + suffix)
            assert 'Invalid' not in out, out
            session.command('soutput.' + suffix)
            actual = (work / ('output.' + suffix)).read_bytes().split(b'\x1a')[0]
            assert actual == data, (actual[:300], data[:300])
            session.command('routput.' + suffix)
            session.command('sagain.' + suffix)
            assert (work / ('again.' + suffix)).read_bytes().split(b'\x1a')[0] == data
        def reject(name, data, message):
            file(name, data)
            contains('r' + name, message)
            contains('?', 'SYMTBL=')
        try:
            code = b'\x90\xb8\x34\x12\xc3' + bytes(11)
            file('sample.cmd', image([(1, 1)], code))
            check('small CMD and known instructions', lambda: (
                session.command('rsample.cmd'), contains('l0,4', 'MOV\tAX,1234H')))
            file('multi.cmd', image([(1, 1), (2, 1)], code + b'ABCDEFGHIJKLMNOP'))
            check('unaligned contiguous groups', lambda: (
                session.command('rmulti.cmd'), contains('dds:0,f', '*ABCDEFGH IJKLMNOP*')))
            file('trail.cmd', image([(1, 1)], code, b'X' * 8192))
            check('ignore bytes beyond described image', lambda: (
                session.command('rtrail.cmd'), contains('l0,4', 'MOV\tAX,1234H')))
            bad = [
                ('type', image([(9, 1)], code)),
                ('repeat', image([(1, 1), (1, 1)], code * 2)),
                ('nocode', image([(2, 1)], code)),
                ('empty', image([(1, 0)], b'')),
                ('nocbody', image([(1, 0), (2, 1)], code)),
                ('large', image([(1, 4097)], code)),
                ('short', image([(1, 9)], code)),
                ('header', b'\x01' * 128),
            ]
            for name, data in bad:
                check('reject ' + name + ' CMD', lambda n=name, d=data:
                      reject(n + '.cmd', d, 'Invalid, truncated or unsupported image'))
            file('plain.com', code + bytes(112))
            check('COM fallback and origin', lambda: (
                session.command('rplain.cmd'), contains('l100,104', 'MOV\tAX,1234H')))
            file('max.com', code + bytes(65280-len(code)))
            check('largest COM', lambda: (
                session.command('rmax.cmd'), contains('l100,104', 'MOV\tAX,1234H')))
            file('big.com', bytes(65408))
            check('reject oversized COM', lambda:
                  contains('rbig.cmd', 'Invalid, truncated or unsupported image'))
            file('zero.com', b'')
            check('reject empty COM', lambda:
                  contains('rzero.cmd', 'Invalid, truncated or unsupported image'))
            check('CMD after COM resets origin', lambda: (
                session.command('rsample.cmd'), contains('?', 'PC    =0000')))
            check('symbol round trip', lambda:
                  roundtrip('sym', b'0000 L START\r\n0010 L FINISH\r\n'))
            check('comment round trip', lambda:
                  roundtrip('doc', b'0000 ;HELLO\r\n0010 *TAIL\r\n'))
            check('control round trip', lambda:
                  roundtrip('ctl', b'C0000,I\r\nD0010,B\r\n'))
            check('overlong comment', lambda:
                  reject('bad.doc', b'0000 '+b'A'*256+b'\r\n\x1a', 'Invalid or oversized'))
            check('short comment after rejected import', lambda:
                  roundtrip('doc', b'0000 ;SHORT\r\n'))
            check('255 character comment', lambda:
                  roundtrip('doc', b'0000 '+b'A'*255+b'\r\n'))
            for suffix in ['doc', 'sym']:
                prefix = b'0000 ' if suffix == 'doc' else b'0000 L '
                check(suffix + ' table capacity', lambda s=suffix, p=prefix:
                      reject('full.'+s, (p+b'A'*200+b'\r\n')*300+b'\x1a', 'Invalid or oversized'))
                check(suffix + ' malformed address', lambda s=suffix:
                      reject('bad.'+s, b'XYZ BAD\r\n\x1a', 'Invalid or oversized'))
            check('control capacity', lambda:
                  reject('full.ctl', b'C0000,I\r\n'*768+b'\x1a', 'Invalid or oversized control'))
            check('control invalid address', lambda:
                  reject('bad.ctl', b'CZZZZ,I\r\n\x1a', 'Invalid or oversized control'))
            check('control truncated record', lambda:
                  reject('bad.ctl', b'C0000,\x1a', 'Invalid or oversized control'))
            check('control maximum capacity', lambda:
                  roundtrip('ctl', b'C0000,I\r\n'*767))
            check('physical EOF after complete record', lambda: (
                file('eof.doc', b'0000 '+b'B'*123), session.command('reof.doc'),
                session.command('seofout.doc'),
                assert_equal((work/'eofout.doc').read_bytes().split(b'\x1a')[0],
                             b'0000 '+b'B'*123+b'\r\n')))
            check('comment sentinel address', lambda:
                  reject('bad.doc', b'FFFF ;BAD\r\n\x1a', 'Invalid or oversized'))
            # Move the comment boundary close to the symbol table, then fill
            # through real interactive insertion (not an assembler test shim).
            stats = session.command('?')
            symbol_base = int(re.search(r'SYMTBL=([0-9A-F]+)', stats)[1], 16)
            file('empty.sym', b'\x1a')
            session.command('rempty.sym')
            check('symbol interactive capacity', lambda: (
                session.command(f'u{symbol_base+32:x}'),
                session.command('e0,.ONE'), session.command('e1,.TWO'),
                session.command('e2,.THREE'), session.command('e3,.FOUR'),
                contains('e4,.FIVE', 'Annotation table full')))
            check('reject comment boundary overlapping symbols', lambda:
                  contains(f'u{symbol_base:x}', '++NO!!'))
            check('interactive comment capacity', lambda: (
                session.command('ufefc'),
                contains(';1,;HELLO', 'Annotation table full'),
                session.command('ud000')))
            check('interactive control capacity', lambda: (
                roundtrip('ctl', b'C0000,I\r\n'*767),
                contains('c1,B', 'Annotation table full')))
            check('recovery of all tables', lambda: (
                roundtrip('sym', b'0000 L START\r\n'),
                roundtrip('doc', b'0000 ;HELLO\r\n'),
                roundtrip('ctl', b'C0000,I\r\n')))
        finally:
            session.close()
    print(f'{version}: {count} cases passed')
    return count


def assert_equal(actual, expected):
    assert actual == expected, (actual, expected)


def read_error_suite():
    """A source-derived diagnostic CMD injects BDOS read error 3 on record 2.

    This is separate from the production-binary suite. All other BDOS calls
    still use emu2; files, command parsing and table maintenance are real.
    """
    import shutil
    with tempfile.TemporaryDirectory(prefix='res86-readerr-') as directory:
        work = Path(directory)
        shutil.copy(ROOT/'res86.a86', work/'res86.a86')
        source = (ROOT/'res86b.a86').read_text()
        source = source.replace('BDOS:\tINT\t0E0H', """BDOS:	CMP CL,20
	JNE REALDOS
	INC CS:BYTE PTR READNUM
	CMP CS:BYTE PTR READNUM,2
	JNE REALDOS
	MOV AL,3
	RET
READNUM DB 0
REALDOS:	INT 0E0H""")
        (work/'res86b.a86').write_bytes(source.replace('\n','\r\n').encode())
        shutil.copy(ROOT/'Makefile', work/'Makefile')
        subprocess.run(['make', 'all'], cwd=work, check=True, stdout=subprocess.DEVNULL)
        count = 0
        for version in ['2.2', '3.1', '4.1']:
            for suffix in ['sym', 'doc', 'ctl']:
                # Fresh process resets the fault counter for each import.
                session = Session(work, version, work/'res86.cmd')
                try:
                    line = {'sym': b'0001 L LABEL', 'doc': b'0001 ;HELLO',
                            'ctl': b'C0001,I'}[suffix] + b'\r\n'
                    (work/('fault.'+suffix)).write_bytes(line*30+b'\x1a')
                    out = session.command('rfault.'+suffix)
                    assert '++UNEXPECTED EOF' in out and 'Invalid or oversized' in out, out
                    session.command('sempty.'+suffix)
                    assert (work/('empty.'+suffix)).read_bytes()[0] == 26
                    # Only read number 2 fails: a later import must recover.
                    (work/('good.'+suffix)).write_bytes(line+b'\x1a')
                    out = session.command('rgood.'+suffix)
                    assert 'Invalid' not in out, out
                    session.command('sgoodout.'+suffix)
                    assert (work/('goodout.'+suffix)).read_bytes().split(b'\x1a')[0] == line
                    count += 1
                finally:
                    session.close()
        print(f'Diagnostic BDOS read-error injection: {count} cases passed')
        return count


if __name__ == '__main__':
    binary = Path(os.environ.get('RES86_CMD', ROOT / 'res86.cmd')).resolve()
    print('Testing', binary)
    total = sum(suite(version, binary) for version in ['2.2', '3.1', '4.1'])
    print(f'{total} production cases passed')
    read_error_suite()
