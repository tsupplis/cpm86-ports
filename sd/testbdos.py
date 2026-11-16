"""Source-derived SD diagnostic builds for boundary and BDOS failure paths."""
from pathlib import Path
import shutil
import re
import subprocess
import tempfile
from test import ROOT, run


def case(version, kind):
    with tempfile.TemporaryDirectory(prefix='sd-bdos-') as directory:
        work = Path(directory)
        source = (ROOT/'sd.a86').read_text().replace('INT\t224', 'CALL\tMOCKDOS').replace('INT 224', 'CALL MOCKDOS')
        count = 5000 if kind == 'capacity' else 1
        drm = 65535 if kind == 'overflow' else 4095
        stub = f'''
DIAGCODE EQU $
        CSEG
        ORG Offset DIAGCODE
MOCKDOS:
        CMP CL,24
        JE MLOG
        CMP CL,25
        JE MGETDRV
        CMP CL,14
        JE MSETDRV
        CMP CL,32
        JE MUSER
        CMP CL,26
        JE MDMA
        CMP CL,31
        JE MDPB
        CMP CL,46
        JE MFREE
        CMP CL,17
        JE MFIRST
        CMP CL,18
        JE MNEXT
        CMP CL,16
        JE MCLOSE
        CMP CL,0
        JE MEXIT
REALDOS: INT 224
        RET
MLOG:   MOV AX,{32772 if kind == 'drives' else 4}
        MOV BX,AX
        RET
MGETDRV:MOV AL,2
        RET
MSETDRV:MOV MDRIVE,DL
        XOR AX,AX
        RET
MUSER:  CMP DL,0FFH
        JE MGETUSR
        MOV MUSR,DL
MGETUSR:MOV AL,MUSR
        RET
MDMA:   MOV MDMAOFF,DX
        INT 224
        RET
MDPB:   MOV AX,DS
        MOV ES,AX
        MOV BX,Offset FAKEDPB
        RET
MFREE:  MOV BX,MDMAOFF
        MOV Word Ptr [BX],8000
        MOV Byte Ptr 2[BX],0
        XOR AX,AX
        RET
MFIRST: CMP DX,Offset CHKFCB
        JNE MSEARCH
        MOV BX,MDMAOFF
        ADD BX,96
        MOV Byte Ptr [BX],20H
        MOV Byte Ptr 15[BX],80H
        MOV Word Ptr 16[BX],8080H
        MOV AX,{3 if kind == 'fat' else 255}
        MOV BX,AX
        RET
MSEARCH: CMP DX,Offset OUTFCB
        JE REALDOS
        MOV MCOUNT,0
MNEXT:  CMP MCOUNT,{count}
        JAE MEOF
        INC MCOUNT
        MOV BX,MDMAOFF
        ADD BX,96
        MOV AL,MUSR
        MOV [BX],AL
        INC BX
        MOV AX,DS
        MOV ES,AX
        MOV SI,Offset FAKENAME
        MOV DI,BX
        PUSH CX
        MOV CX,11
        CLD
        REP MOVSB
        POP CX
        MOV Byte Ptr 11[BX],0
        MOV Byte Ptr 12[BX],0
        MOV Byte Ptr 13[BX],0
        MOV Byte Ptr 14[BX],1
        MOV AX,{4 if kind == 'search' else 3}
        MOV BX,AX
        RET
MEOF:   MOV AX,255
        MOV BX,AX
        RET
MCLOSE: CMP DX,Offset OUTFCB
        JNE REALDOS
        {'MOV AX,255' if kind == 'close' else 'JMP REALDOS'}
        MOV BX,AX
        RET
MEXIT:  CMP MUSR,5
        JNE MFAIL
        CMP MDRIVE,2
        JNE MFAIL
        INT 224
        RET
MFAIL:  MOV DX,Offset FAILMSG
        MOV CL,9
        INT 224
        MOV CL,0
        XOR DL,DL
        INT 224
DIAGEND EQU $
        DSEG
        ORG Offset DIAGEND
MDRIVE DB 2
MUSR DB 5
MDMAOFF DW 80H
MCOUNT DW 0
FAKEDPB DW 26
        DB 3,7,0
        DW 255,{drm}
        DB 0C0H,0
        DW 0,2
FAKENAME DB 'ALPHA   TXT'
FAILMSG DB 'STATE RESTORE FAILED','$'
'''
        jumpno = [0]
        def longbranch(match):
            jumpno[0] += 1
            label = 'DJ'+str(jumpno[0])
            inverse = {'JE':'JNE', 'JNE':'JE', 'JAE':'JB'}[match[1]]
            return '        '+inverse+' '+label+'\n        JMP '+match[2]+'\n'+label+':'
        stub = re.sub(r'        (JE|JNE|JAE) (\w+)',longbranch,stub)
        source = source.replace('ORDER\tEQU\t(Offset $)', stub+'\nORDER\tEQU\t(Offset $)')
        (work/'sd.a86').write_bytes(source.replace('\n','\r\n').encode())
        shutil.copy(ROOT/'Makefile',work/'Makefile')
        build = subprocess.run(['make','all'],cwd=work,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        assert build.returncode == 0,build.stdout.decode()
        opts = {'close':'[FN]', 'users':'[AN]', 'drives':'[DN]'}.get(kind,'[NU1]')
        out = run(work/'sd.cmd',work,['B:*.TXT',opts],version)
        assert 'STATE RESTORE FAILED' not in out,out
        expected = {'overflow':'Memory Error','capacity':'Memory Error',
                    'search':'Directory search Error','close':'Close Error',
                    'fat':'DOS','state':'ALPHA','users':'User 15','drives':'Drive P'}[kind]
        assert expected in out,(kind,out)
        if kind == 'state':
            assert 'DOS' not in out,out  # unsuccessful probe contains stale FAT bytes
        return 1


def suite():
    total=0
    for version in ['2.2','3.1','4.1']:
        for kind in ['overflow','capacity','search','close','fat','state','users','drives']:
            total += case(version,kind)
        print(version+': 8 diagnostic cases passed',flush=True)
    print(f'{total} diagnostic cases passed')


if __name__ == '__main__':
    suite()
