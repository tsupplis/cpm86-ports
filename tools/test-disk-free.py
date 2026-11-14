"""DISK7 free-space boundaries using explicit test-only BDOS responses.

The emulator is unchanged. Function 46 success/overflow are controlled contract
fixtures; the shipped program is also run against its real unsupported response.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
folder = root / 'disk7'


def command(args, work, **kwargs):
    result = subprocess.run(args, cwd=work, capture_output=True, timeout=30, **kwargs)
    assert result.returncode == 0, (args, result.stdout, result.stderr)
    return result.stdout


def write(path, text):
    path.write_bytes(text.replace('\r\n', '\n').replace('\n', '\r\n').encode('latin1'))


def run_case(version, records, expected, error=False, blocks=8, shift=3):
    with tempfile.TemporaryDirectory(prefix='disk-free-') as directory:
        work = Path(directory)
        for source in (folder / 'disk7.a86', *folder.glob('*.inc')):
            shutil.copy2(source, work / source.name)
        entry = (work / 'disk7.a86').read_text()
        # Enter the real FRESTOR with a deliberately nondefault DMA address.
        old = "\tCALL\tHELP\t\t\t;show 'menu'"
        assert old in entry
        entry = entry.replace(old, '''
 MOV DX,Offset TESTDMA
 MOV CL,26
 INT 224
 MOV Word Ptr TESTOFF,Offset TESTDMA
 CALL FRESTOR
 MOV CL,0
 XOR DX,DX
 INT 224
''', 1)
        write(work / 'disk7.a86', entry)
        utility = (work / 'd7util.inc').read_text()
        start, end = utility.index('FRESTOR:'), utility.index('; HIDDEN is an in-memory flag')
        section = utility[start:end]
        assert section.count('\tINT\t224') == 6
        utility = utility[:start] + section.replace('\tINT\t224', '\tCALL\tTESTBDOS') + utility[end:]
        # Every bitmap bit is free; includes the 65536-block counter boundary.
        mock = f'''
TESTBDOS:
 CMP CL,25
 JNE TEST31
 XOR AX,AX
 RET
TEST31:
 CMP CL,31
 JNE TEST12
 MOV AX,DS
 MOV ES,AX
 MOV BX,Offset TESTDPB
 RET
TEST12:
 CMP CL,12
 JNE TEST26
 MOV BX,{version}
 MOV AL,BL
 RET
TEST26:
 CMP CL,26
 JNE TEST46
 MOV Word Ptr TESTOFF,DX
 INT 224
 RET
TEST46:
 CMP CL,46
 JNE TEST27
 CMP Word Ptr TESTOFF,128
 JE TESTDMAOK
 MOV DX,Offset TESTFAIL
 MOV CL,9
 INT 224
 MOV CL,0
 XOR DX,DX
 INT 224
TESTDMAOK:
'''
        if error:
            mock += ' MOV AL,255\n RET\n'
        else:
            mock += f''' MOV DI,Word Ptr TESTOFF
 MOV Word Ptr [DI],{records & 65535}
 MOV Byte Ptr 2[DI],{records >> 16}
 XOR AX,AX
 RET
'''
        mock += f'''
TEST27:
 MOV AX,DS
 MOV ES,AX
 MOV BX,Offset TESTMAP
 RET
TESTOFF DW 0
TESTDPB DB 8,0,{shift},7,0
 DW {blocks - 1}
 DB 0,0,0,0,0,0,0,0
TESTFAIL DB 'TEST FAILED: wrong DMA$'
TESTDMA RS 128
TESTMAP RS {(blocks + 7) // 8}
'''
        # Explicitly zero the bitmap; reserved storage need not be initialized.
        entry = (work / 'disk7.a86').read_text().replace(' CALL FRESTOR', f'''
 MOV BX,Offset TESTMAP
 MOV CX,{(blocks + 7) // 8}
TESTZERO:
 MOV Byte Ptr [BX],0
 INC BX
 LOOP TESTZERO
 CALL FRESTOR''', 1)
        write(work / 'disk7.a86', entry)
        utility = utility.replace('; HIDDEN is an in-memory flag', mock + '\n; HIDDEN is an in-memory flag', 1)
        write(work / 'd7util.inc', utility)
        log = command(['cpm86_asm86', 'disk7'], work)
        assert b'NUMBER OF ERRORS:   0' in log and b'ERROR NO:' not in log, log
        command(['cpm86_gencmd', 'disk7.h86', '8080', 'CODE[M400,XFFF]'], work)
        output = command(['emu2', 'disk7.cmd'], work,
                         env=dict(os.environ, EMU2_CPM_VER='2.2', EMU2_DRIVE_C=str(work), EMU2_DEFAULT_DRIVE='C:'))
        assert output.strip().startswith(expected) and b'TEST FAILED' not in output, output
        print(f'version={version:02x} records={records} blocks={blocks} shift={shift} error={error}: {expected.decode()}')


for records, expected in ((0, b'0k bytes free'), (7, b'0k bytes free'),
                          (8, b'1k bytes free'), (65535 * 8 + 7, b'65535k bytes free'),
                          (65536 * 8, b'>65535k bytes free'), (0xffffff, b'>65535k bytes free')):
    run_case(0x31, records, expected)
run_case(0x31, 0, b'Free space unavailable', error=True)
for blocks, shift, expected in ((8, 3, b'8k bytes free'), (32767, 4, b'65534k bytes free'),
                                (32768, 4, b'>65535k bytes free'),
                                (65535, 3, b'65535k bytes free'),
                                (65536, 3, b'>65535k bytes free')):
    run_case(0x22, 0, expected, blocks=blocks, shift=shift)

# Actual unmodified CMD, against the current emulator's missing function 46.
with tempfile.TemporaryDirectory(prefix='disk-free-native-') as directory:
    work = Path(directory)
    (work / 'alpha.dat').write_bytes(b'ALPHA\r\n\x1a')
    output = command(['emu2', str(folder / 'disk7.cmd'), '*.dat'], work,
                     input=b'X', env=dict(os.environ, EMU2_CPM_VER='3.1',
                                         EMU2_DRIVE_C=str(work), EMU2_DEFAULT_DRIVE='C:'))
    assert b'Free space unavailable' in output and b'ALPHA' in output, output
    assert (work / 'alpha.dat').read_bytes() == b'ALPHA\r\n\x1a'
    print('unmodified CMD: unsupported function 46 reports unavailable and browsing still works')
