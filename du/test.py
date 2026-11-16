"""Execute source-derived disk-backend routines with a private fake BIOS.

No host disk/image is exposed to the BIOS stub. Production raw disk access
requires separate testing on disposable PCE images.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def check(name, body, source):
    routines = source[source.index('SECT_TO_MEM:'):source.index('BIOSIOPB:')]
    routines += '\nJMP BIOSIOPB\n' + source[source.index('\nREAD:\t'):source.index(';Help', source.index('\nREAD:\t'))]
    routines += source[source.index('CPMCHK:'):source.index('; direct bios call for cp/m-86 version 1.1')].replace('INT\t224', 'CALL VERDOS')
    fat = source[source.index('\tCMP\tWORD PTR[SI],0FFFFH'):source.index('NOT_PC_DPB:')]
    routines += '\nFATCHECK:\nPUSH ES\nPUSH DS\nMOV SI,OFFSET FAKEDPB\n' + fat.replace('INT\t224','CALL VERDOS') + '\nNOT_PC_DPB:\nPOP DS\nPOP ES\nRET\n'
    asm = """CSEG
ORG 100H
JMP START
CR EQU 13
LF EQU 10
BIO_READ EQU 10
BIO_WRITE EQU 11
START:
MOV AX,CS
MOV DS,AX
MOV ES,AX
MOV SS,AX
MOV SP,0F000H
MOV DMASEG,AX
MOV IDMASEG,AX
CLD
MOV DI,OFFSET SECTOR_BUF
MOV CX,512
MOV AL,55H
REP STOSB
MOV DI,OFFSET USERBUF
MOV CX,128
MOV AL,33H
REP STOSB
""" + body + """
MOV DX,OFFSET PASSMSG
JMP DONE
FAIL:
MOV DX,OFFSET FAILMSG
DONE:
MOV CL,9
INT 224
MOV CL,0
XOR DL,DL
INT 224
""" + routines + """
VERDOS:
CMP CL,12
JNE VRETURN
MOV AX,VERSION
VRETURN:
RET
EXIT:
MOV REJECTED,1
RET
VERSION DW 1022H
CPM_VERSION DW 0
REJECTED DB 0
BIOS_CALL_TBL DW 0
TABLE11 DB 0
TABLE31 DB 0
WVMSG DB '$'
MPMSG DB '$'
FATMSG DB '$'
FAKEDPB DW 0
DSKREAD:
MOV AL,ERROR
RET
DSKWRITE:
INC WRITES
MOV AL,ERROR
RET
EXPL:
RET
ILPRT:
POP BP
PUSH AX
SKIPMSG:
MOV AL,DS:[BP]
INC BP
OR AL,AL
JNZ SKIPMSG
POP AX
PUSH BP
RET
WRFLG DB 0
QFLAG DB 0
BIOSIOPB:
CMP BX,BIO_READ
JNE WROTE
INC READS
MOV AL,ERROR
RET
WROTE:
INC WRITES
XOR AX,AX
RET
TRK_SECT DW 0FFFFH,0
ITRACK DW 1
ISECTOR DW 2
REC_OFFSET DW 0
DMASEG DW 0
DMAOFF DW OFFSET USERBUF
IDMASEG DW 0
READS DW 0
WRITES DW 0
ERROR DB 0
PASSMSG DB 'PASS$'
FAILMSG DB 'FAIL$'
SECTOR_BUF RB 512
USERBUF RB 128
END
"""
    # 8086 conditional branches have an 8-bit displacement.
    import re
    seq = [0]
    def branch(m):
        seq[0] += 1
        label = 'CHECK' + str(seq[0])
        inv = {'JNE':'JE', 'JE':'JNE'}[m[1]]
        return inv+' '+label+'\nJMP FAIL\n'+label+':'
    asm = re.sub(r'(JNE|JE) FAIL', branch, asm)
    with tempfile.TemporaryDirectory(prefix='du-test-') as directory:
        work = Path(directory)
        (work/'diag.a86').write_bytes(asm.replace('\n','\r\n').encode())
        for command in [['cpm86_asm86','diag.a86'],
                        ['cpm86_gencmd','diag.h86','8080','CODE[MF00]']]:
            result = subprocess.run(command,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
            assert result.returncode == 0 and (command[0] != 'cpm86_asm86' or (work/'diag.h86').exists()),result.stdout.decode(errors='replace')
        result = subprocess.run([os.environ.get('EMU2','emu2'),'-c','8086',str(work/'diag.cmd')],cwd=work,
                                stdout=subprocess.PIPE,stderr=subprocess.STDOUT,input=b'',timeout=5)
        assert result.returncode == 0 and b'PASS' in result.stdout,(name,result.stdout)
    print('PASS '+name,flush=True)


def main():
    source = (ROOT/'du.a86').read_text().split('\x1a')[0]
    cases = {
        'failed read disables write': 'MOV WRFLG,1\nMOV ERROR,1\nCALL READ\nCALL WRITE\nCMP WRITES,0\nJNE FAIL',
        'successful read permits write': 'CALL READ\nCALL WRITE\nCMP WRITES,1\nJNE FAIL',
        'failed write disables retry': 'MOV WRFLG,1\nMOV ERROR,1\nCALL WRITE\nCALL WRITE\nCMP WRITES,1\nJNE FAIL\nCMP TRK_SECT,0FFFFH\nJNE FAIL',
        'retry failed read': 'MOV ERROR,1\nCALL SECT_TO_MEM\nCALL SECT_TO_MEM\nCMP READS,2\nJNE FAIL',
        'abort write after failed read': 'MOV ERROR,1\nCALL WRITE_SECTOR31\nCMP WRITES,0\nJNE FAIL\nCMP AL,1\nJNE FAIL',
        'failed read preserves user buffer': 'MOV ERROR,1\nCALL READ_SECTOR31\nCMP USERBUF,33H\nJNE FAIL\nCMP AL,1\nJNE FAIL',
        'reuse successful read': 'CALL SECT_TO_MEM\nCALL SECT_TO_MEM\nCMP READS,1\nJNE FAIL',
        'track zero sector change': 'MOV ITRACK,0\nCALL SECT_TO_MEM\nINC ISECTOR\nCALL SECT_TO_MEM\nCMP READS,2\nJNE FAIL',
        'write preserves other records': 'MOV REC_OFFSET,1\nCALL WRITE_SECTOR31\nCMP WRITES,1\nJNE FAIL\nCMP SECTOR_BUF,55H\nJNE FAIL\nCMP BYTE PTR SECTOR_BUF+128,33H\nJNE FAIL\nCMP BYTE PTR SECTOR_BUF+256,55H\nJNE FAIL',
    }
    for version in ['0022','1022','1031','1431','1441','1041','1450','1122']:
        expected = 0 if version in ['0022','1022','1031','1431','1441'] else 1
        cases['version '+version] = 'MOV VERSION,0'+version+'H\nCALL CPMCHK\nCMP REJECTED,'+str(expected)+'\nJNE FAIL'
    cases['reject DOS DPB'] = 'MOV FAKEDPB,0FFFFH\nCALL FATCHECK\nCMP REJECTED,1\nJNE FAIL'
    cases['accept CP/M DPB'] = 'MOV FAKEDPB,26\nCALL FATCHECK\nCMP REJECTED,0\nJNE FAIL'
    failed=[]
    for name,body in cases.items():
        try:
            check(name,body,source)
        except AssertionError as error:
            failed.append(name)
            print('FAIL',error,flush=True)
    print(str(len(cases)-len(failed))+' passed; '+str(len(failed))+' failed')
    if failed:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
