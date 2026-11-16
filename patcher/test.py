"""Build a private configured patcher and verify actual file contents."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent

def suite(version):
    with tempfile.TemporaryDirectory(prefix='patcher-') as tmp:
        w = Path(tmp)
        source = (ROOT / 'patcher.a86').read_text().split('hello:')[0]
        source += """hello db 'Test patch',13,10,0
files db 2
fname db 0,'TEST.BIN',0,0,'SECOND.BIN',0
bytes dw 4,1
offs dw 0,0,0,127,0,128,1,1,0,0
value db 65,66,65,67,65,68,65,69,65,70
end
"""
        (w/'patcher.a86').write_bytes(source.replace('\n','\r\n').encode())
        shutil.copy(ROOT/'Makefile', w)
        subprocess.run(['make'], cwd=w, check=True)
        env = dict(os.environ, EMU2_CPM_VER=version, EMU2_DRIVE_C=tmp, EMU2_DEFAULT_DRIVE='C:')
        def run():
            r = subprocess.run([os.environ.get('EMU2','emu2'), '-c','8086',str(w/'patcher.cmd')], cwd=w, env=env, input=b'', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
            assert r.returncode == 0, r.stdout
            return r.stdout.decode(errors='replace')
        original = b'A'*65664
        (w/'test.bin').write_bytes(original)
        (w/'second.bin').write_bytes(b'X'*128)
        out = run()
        assert 'PATCH FAILED' in out, out
        assert (w/'test.bin').read_bytes() == original
        (w/'second.bin').write_bytes(b'A'*128)
        out = run()
        assert 'Patch successfully installed' in out, out
        expected = bytearray(original)
        for offset, value in [(0,66),(127,67),(128,68),(65537,69)]:
            expected[offset] = value
        assert (w/'test.bin').read_bytes() == expected, out
        assert (w/'second.bin').read_bytes() == b'F'+b'A'*127, out
        out = run()
        assert 'already patched' in out and 'PATCH FAILED' not in out, out
        assert (w/'test.bin').read_bytes() == expected
        print(version + ': 3 cases passed (preflight rejection, exact patching, repeat run)')

for version in ['2.2','3.1']:
    suite(version)
