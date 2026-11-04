"""Exercise the real WASH15/DISK7 code with smaller CP/M-86 allocations."""
from pathlib import Path
import os
import re
import struct
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parent.parent
program=sys.argv[1]
assert program in ('wash15','disk7'),program
folder=root/program
image=(folder/(program+'.cmd')).read_bytes()
listing=(folder/(program+'.lst')).read_text()


def address(text,label):
    lines=text.splitlines()
    pattern=re.compile(r'(?<![A-Za-z0-9_@])'+re.escape(label)+
                       r'(?:\s*:|\s+(?:RS|DW|EQU)\b)')
    for i,line in enumerate(lines):
        if pattern.search(line):
            for entry in lines[i:]:
                match=re.match(r'^\s*([0-9A-F]{4})\s',entry)
                if match:
                    return int(match[1],16)
    raise AssertionError('missing symbol '+label)


def hexword(value):
    return '0%04XH'%value


def jump(origin,destination):
    return b'\xe9'+struct.pack('<H',(destination-origin-3)&0xffff)


wash=program=='wash15'
base=address(listing,'FNLIST' if wash else 'RING')
stride=13 if wash else 14
memtop=address(listing,'MEMTOP')
buffer_count=address(listing,'BUFMAX' if wash else 'REC@MAX')
entry=0x100 if wash else address(listing,'DISK7')
exit_start=address(listing,'EXIT' if wash else 'CPM@CCP')
exit_end=address(listing,'RENAME' if wash else 'HELP')
exit_at=image.rfind(b'\xcd\xe0',128+exit_start,128+exit_end)-128
assert exit_at>=exit_start
menu_call=re.search(r'^\s*([0-9A-F]{4})\s+[0-9A-F]+\s+[0-9A-F]{4}\s+CALL\s+'+
                    ('MENU' if wash else 'HELP')+r'\b',listing,re.M)
assert menu_call
menu_call=int(menu_call[1],16)
banner=image.index(b' MICRO RESOURCES DIRECTORY' if wash else b'                 DISK ')
banner_end=image.index(b'\0\xc3',banner)
stub_origin=banner-128
payload=b''.join(bytes([i])*128 for i in range(20))


def run(limit,count,copy=False,read_error=None):
    effective=min(limit,0xffff)
    overflow=base+count*stride+128>effective
    expected_records=(effective-base-count*stride)//128
    with tempfile.TemporaryDirectory(prefix=program+'-bounds-') as name:
        work=Path(name)
        source=work/'source'
        destination=work/'dest'
        source.mkdir()
        destination.mkdir()
        for i in range(count):
            (source/('F%07d.TST'%i)).write_bytes(payload)

        # emu2 grants 8080-model programs at least 64 KB. As in the EDLIN
        # tests, override the base page only in a disposable test CMD.
        setup=f'''
        CSEG
        ORG {hexword(stub_origin)}
        MOV Word Ptr .0,{limit&0xffff}
        MOV Byte Ptr .2,{limit>>16}
        MOV Word Ptr .6,0
'''
        guard_setup='' if limit>=65536 else f'''
        MOV BX,{hexword(limit)}
        MOV CX,16
FILL_GUARD:
        MOV Byte Ptr [BX],0A5H
        INC BX
        LOOP FILL_GUARD
'''
        guard_check='' if limit>=65536 else f'''
        MOV BX,{hexword(limit)}
        MOV CX,16
CHECK_GUARD:
        CMP Byte Ptr [BX],0A5H
        JNE FAILED
        INC BX
        LOOP CHECK_GUARD
'''
        count_check='' if overflow or (wash and not copy) else f'''
        CMP Word Ptr .{hexword(buffer_count)},{expected_records}
        JNE FAILED
'''
        mock_read='' if read_error is None else f'''
MOCK_READ:
        INC Byte Ptr READ_COUNT
        CMP Byte Ptr READ_COUNT,{read_error[0]}
        JE READ_FAILED
        INT 224
        OR AL,AL
        RET
READ_FAILED:
        MOV AL,{read_error[1]}
        OR AL,AL
        RET
READ_COUNT DB 0
'''
        stub=setup+guard_setup+'''
        DB '''+','.join(str(b) for b in image[128+entry:128+entry+3])+f'''
        DB 0E9H
        DW {hexword(entry+3)}-(Offset $)-2
STUB_CHECK:
        CMP Word Ptr .{hexword(memtop)},{hexword(effective)}
        JNE FAILED
'''+guard_check+count_check+'''
        MOV DX,Offset OKMSG
        JMPS PRINT_RESULT
FAILED:
        MOV DX,Offset BADMSG
PRINT_RESULT:
        MOV CL,9
        INT 224
        XOR DX,DX
        MOV CL,0
        INT 224
OKMSG DB 13,10,'PASS: allocation guard and buffer size',13,10,'$'
BADMSG DB 13,10,'FAIL: allocation guard or buffer size',13,10,'$'
'''+mock_read+'''
STUB_END RS 0
        END
'''
        (work/'STUB.A86').write_bytes(stub.replace('\n','\r\n').encode())
        for command in (['cpm86_asm86','STUB.A86'],
                        ['cpm86_gencmd','STUB.h86','8080']):
            result=subprocess.run(command,cwd=work,capture_output=True,timeout=30)
            assert result.returncode==0,(command,result.stdout,result.stderr)
        stub_listing=(work/'STUB.lst').read_text()
        end=address(stub_listing,'STUB_END')
        checker=address(stub_listing,'STUB_CHECK')
        assert 128+end<=banner_end,(end,banner_end-128)
        stub_image=(work/'STUB.cmd').read_bytes()
        test_image=bytearray(image)
        test_image[banner:128+end]=stub_image[banner:128+end]
        test_image[128+entry:128+entry+3]=jump(entry,stub_origin)
        test_image[128+menu_call:128+menu_call+3]=b'\x90'*3
        test_image[128+exit_at:128+exit_at+3]=jump(exit_at,checker)
        if read_error is not None:
            assert wash
            start=address(listing,'COPY7')
            end_read=address(listing,'COPY7_DATA')
            read_at=image.rfind(b'\xcd\xe0\x0a\xc0',128+start,128+end_read)-128
            assert read_at>=start
            mock=address(stub_listing,'MOCK_READ')
            test_image[128+read_at:128+read_at+4]=(
                b'\xe8'+struct.pack('<H',(mock-read_at-3)&0xffff)+b'\x90')
        (source/'CHECK.CMD').write_bytes(test_image)
        commands=(b'CBX' if wash else b'CB:\nX') if copy else b'X'
        env=dict(os.environ,EMU2_DEFAULT_DRIVE='A:',
                 EMU2_DRIVE_A=str(source),EMU2_DRIVE_B=str(destination))
        result=subprocess.run(['emu2',str(source/'CHECK.CMD'),'*.TST'],cwd=source,
                              env=env,input=commands,capture_output=True,timeout=15)
        text=(result.stdout+result.stderr).decode('latin1')
        assert 'PASS: allocation guard and buffer size' in text,(limit,count,copy,text)
        assert 'FAIL:' not in text,(limit,count,copy,text)
        assert ('Directory too large for memory' in text)==overflow,(limit,count,copy,text)
        if not overflow:
            assert 'F0000000.TST' in text.replace(' ',''),(limit,count,copy,text)
        if copy and read_error is None:
            copied=next(p for p in destination.iterdir() if p.name.upper()=='F0000000.TST')
            assert copied.read_bytes()==payload,(limit,count,text)
        if read_error is not None:
            assert 'Source Read Error - Copy Aborted' in text,text
            assert 'Cannot Be Deleted' not in text and 'Close Error' not in text,text
            assert not list(destination.iterdir()),text
        assert all(p.read_bytes()==payload for p in source.glob('*.TST'))
        print('%s: allocation=%d files=%d %s passed'%
              (program,limit,count,'read error '+str(read_error) if read_error else
               'copy' if copy else 'overflow' if overflow else 'browse'))


run(base+128+stride-1,1)
run(base+128+stride,1,copy=True)
run(base+128+3*stride,3,copy=True)
run(base+128+3*stride,4)
run(8192,3,copy=True)
run(16384,3,copy=True)
run(65536,3,copy=True)
if wash:
    for failure in ((1,2),(3,2),(3,255)):
        run(base+128+3*stride,3,copy=True,read_error=failure)

with tempfile.TemporaryDirectory(prefix=program+'-normal-') as name:
    work=Path(name)
    destination=work/'dest'
    destination.mkdir()
    (work/'F0000000.TST').write_bytes(payload)
    env=dict(os.environ,EMU2_DEFAULT_DRIVE='A:',
             EMU2_DRIVE_A=str(work),EMU2_DRIVE_B=str(destination))
    result=subprocess.run(['emu2',str(folder/(program+'.cmd')),'*.TST'],cwd=work,
                          env=env,input=b'CBX' if wash else b'CB:\nX',
                          capture_output=True,timeout=15)
    text=(result.stdout+result.stderr).decode('latin1')
    copied=next(p for p in destination.iterdir() if p.name.upper()=='F0000000.TST')
    assert copied.read_bytes()==payload,text
    assert (work/'F0000000.TST').read_bytes()==payload,text
    assert 'Directory too large' not in text and 'error' not in text.lower(),text
    assert ('MICRO RESOURCES' if wash else 'DISK 7.7') in text,text
    print(program+': unmodified CMD startup, menu and single-file copy passed')
