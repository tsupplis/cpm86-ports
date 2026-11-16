"""Production CMD checks with private files; no PCE in the make target."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def run(binary, work, args=(), version='2.2'):
    env = dict(os.environ, EMU2_CPM_VER=version, EMU2_DRIVE_C=str(work),
               EMU2_DEFAULT_DRIVE='C:')
    result = subprocess.run([os.environ.get('EMU2', 'emu2'), '-c', '8086',
                             str(binary), *args], cwd=work, env=env, input=b'',
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=10)
    assert result.returncode == 0, result.stdout
    return result.stdout.decode('ascii', errors='replace')


def production():
    total = 0
    for version in ['2.2', '3.1', '4.1']:
        with tempfile.TemporaryDirectory(prefix='sd-test-') as directory:
            work = Path(directory)
            for name, size in [('zeta.txt', 128), ('alpha.txt', 128),
                               ('alpha.bin', 4096), ('beta.txt', 20000)]:
                (work/name).write_bytes(b'A'*size)
            binary = ROOT/'sd.cmd'
            outputs = [run(binary, work, ['*.txt', options], version)
                       for options in ['[NS', '[NS]', '[N,S]', '[N S]']]
            assert len(set(outputs)) == 1, outputs
            out = outputs[0]
            assert out.index('ALPHA') < out.index('BETA') < out.index('ZETA'), out
            assert '3 files' in out and '20k' in out, out
            total += 4
            for options in ['[N', '[N]']:
                out = run(binary, work, [options], version)
                assert 'ALPHA' in out and 'ZETA' in out, out
                total += 1
            out = run(binary, work, ['*.txt', '[DN]'], version)
            assert 'Drive C' in out and 'ALPHA' in out,out
            total += 1
            out = run(binary, work, ['*.txt', '[W]'], version)
            assert 'Options Are:' in out and '?' in out, out
            total += 1
            # Existing-file append needs CP/M FCB record counts; validate it
            # in testpce.py, not through emu2's DOS-layout open delegation.
            run(binary, work, ['*.txt', '[FN]'], version)
            data = (work/'sd.dir').read_bytes().split(b'\x1a')[0]
            assert b'ALPHA' in data and b'BETA' in data and b'ZETA' in data
            total += 1
            (work/'large.bin').write_bytes(b'X'*(600*1024))
            out = run(binary, work, ['large.bin','[N]'], version)
            assert '600k' in out and '1 files' in out,out
            total += 1
            # Malformed and truncated library directories must stop at actual EOF.
            for length in [0, 128]:
                body = bytearray(length)
                if length:
                    body[1:12] = b' '*11
                    body[14:16] = b'\xff\xff'
                (work/'bad.lbr').write_bytes(body)
                out = run(binary, work, ['bad.lbr', '[LN]'], version)
                assert 'Not a LIBRARY' in out, out
                total += 1
            print(f'{version}: 12 production cases passed', flush=True)
    return total


if __name__ == '__main__':
    print(f'{production()} production cases passed')
