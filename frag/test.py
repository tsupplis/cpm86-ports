"""Directory fixtures through a private BDOS shim; execute FRAG's real logic."""
import os
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = (ROOT/'frag.a86').read_text()


def case(version, fat=False, words=False, count=2, empty=False):
    with tempfile.TemporaryDirectory(prefix='frag-') as tmp:
        w = Path(tmp)
        source = SOURCE.replace('INT     224', 'CALL    MOCKBDOS')
        # Explicit B: selection must occur before probing; exit restores A:.
        source = source.replace('FRAG:\n', 'FRAG:\n                MOV FCBDN,2\n')
        stub = f'''
MOCKBDOS:
                CMP CL,25
                JE mock_current
                CMP CL,14
                JE mock_select
                CMP CL,12
                JE mock_version
                CMP CL,31
                JE mock_dpb
                CMP CL,17
                JE mock_first
                CMP CL,18
                JE mock_next
                CMP CL,0
                JE mock_exit
                INT 224
                RET
mock_current:
                XOR AX,AX
                RET
mock_select:
                MOV mock_drive,DL
                XOR AX,AX
                RET
mock_version:
                MOV AX,{version}
                RET
mock_dpb:
                CMP mock_drive,1
                JNE mock_fail
                MOV AX,DS
                MOV ES,AX
                MOV BX,Offset fake_dpb
                RET
mock_first:
                CMP mock_drive,1
                JNE mock_fail
                CMP DX,Offset Raw_FCB
                JNE mock_files
                MOV DI,Offset BUFF+96
                MOV CX,16
                XOR AX,AX
                REP STOSW
                MOV Byte Ptr BUFF+96,{32 if fat else 0}
                MOV Byte Ptr BUFF+111,80H
                MOV Word Ptr BUFF+112,8080H
                MOV AL,3
                RET
mock_files:
                MOV mock_count,0
mock_next:
                CMP mock_count,{0 if empty else count}
                JAE mock_eof
                CMP mock_count,256
                JAE mock_entry
                MOV AX,mock_count
                MOV BX,EntryLength
                MUL BX
                ADD AX,Offset Dir_Array
                CMP Last_Entry,AX
                JNE mock_fail
mock_entry:
                INC mock_count
                MOV DI,Offset BUFF+96
                MOV CX,16
                XOR AX,AX
                REP STOSW
                MOV SI,Offset fake_name
                MOV DI,Offset BUFF+97
                MOV CX,11
                REP MOVSB
                MOV Byte Ptr BUFF+111,80H
                MOV Word Ptr BUFF+112,0201H
                MOV Word Ptr BUFF+114,0403H
                MOV AL,3
                RET
mock_eof:
                MOV AL,0FFH
                RET
mock_exit:
                CMP mock_drive,0
                JNE mock_fail
                INT 224
                RET
mock_fail:
                MOV DX,Offset failmsg
                MOV CL,9
                INT 224
                MOV CL,0
                XOR DL,DL
                INT 224
'''
        serial = [0]
        def far_branch(m):
            serial[0] += 1
            label = 'mock_jump_' + str(serial[0])
            opposite = {'JE':'JNE','JNE':'JE','JAE':'JB'}[m[1]]
            return '                '+opposite+' '+label+'\n                JMP '+m[2]+'\n'+label+':'
        stub = re.sub(r'                (JE|JNE|JAE) (mock_\w+)',far_branch,stub)
        source = source.replace('                DSEG\n', stub+'\n                DSEG\n',1)
        source = source.replace('Old_Drive       DB', f'''mock_drive DB 0
mock_count DW 0
fake_dpb DW 32
         DB 3,7,0
         DW {300 if words else 157}
fake_name DB 'TEST    BIN'
failmsg DB 'HARNESS FAILURE$'
Old_Drive       DB''',1)
        (w/'frag.a86').write_bytes(source.replace('\n','\r\n').encode())
        (w/'Makefile').write_bytes((ROOT/'Makefile').read_bytes())
        b = subprocess.run(['make'],cwd=w,capture_output=True)
        assert b.returncode == 0,b.stdout+b.stderr
        r = subprocess.run([os.environ.get('EMU2','emu2'),str(w/'frag.cmd')],cwd=w,
                           input=b'',capture_output=True,timeout=10)
        out=r.stdout.decode(errors='replace')
        assert r.returncode == 0 and 'HARNESS FAILURE' not in out,out
        if fat:
            assert 'DOS/FAT disks are not supported' in out and 'TEST' not in out,out
        elif count>256:
            assert 'Too many extents' in out,out
        elif empty:
            assert 'No file(s) found' in out,out
        else:
            assert '0201 0403' in out if words else '0102 0304' in out,out
            assert out.count('80   ') == count,out

for version in (0x22,0x1431,0x1441):
    case(version)
    case(version,words=True)
    case(version,empty=True)
case(0x1441,fat=True)
case(0x1441,count=256)
case(0x1441,count=257)
print('PASS: 12 cases: byte/word maps, empty directories, FAT rejection, 256/257 bounds and drive restoration')
