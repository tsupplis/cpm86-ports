#!/usr/bin/env python3
"""Execute FILER's actual CHAIN/P_CLI assembler with mocked BDOS calls."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parent
text=(root/'filer.a86').read_text()
routine=text[text.index('\nCHAIN:\n')+1:text.index('\nUC_CMD:')]
routine=routine.replace('INT    224','CALL   mockbdos')
source='''        cseg
        org 100h
TBUF equ 80h
        jmp begin
DATA_SEG dw 0
CCKEYS db 1
CLIERR db 0
CLIUSER db 0
CLIDRIVE db 0
CLIPRIO db 0
CLICB db 0
      rs 129
pd db 0,0,0,0,0,200
   rs 24
   dw parent
parent db 0,0,0,0,0,198
stage db 0
user db 0
drive db 1
priority db 200
launches db 0
chains db 0
attaches db 0
refreshes db 0
messages db 0
keys db 1
fail_launch db 0
wantprio db 197
; concurrent, parent priority, launch error, expected wait priority
cases db 1,198,0,197, 1,198,1,197, 1,197,0,196, 1,64,0,0, 0,198,0,0
command db 'DIR A:',0
ok db 'PASS: FILER P_CLI/chain, nested priority, console recovery,',13,10
   db 'context restoration, launch failure and repeated resident returns',13,10,'$'
bad db 'FAIL: FILER execution',13,10,'$'
begin:
        push cs
        pop ds
        push cs
        pop es
        mov ax,ds
        mov DATA_SEG,ax
        mov bx,offset cases
        xor ax,ax
        mov al,stage
        shl ax,1
        shl ax,1
        add bx,ax
        mov al,[bx]
        mov CCKEYS,al
        mov keys,al
        mov al,1[bx]
        mov parent+5,al
        mov al,2[bx]
        mov fail_launch,al
        mov al,3[bx]
        mov wantprio,al
        mov user,0
        mov drive,1
        mov priority,200
        mov launches,0
        mov chains,0
        mov attaches,0
        mov refreshes,0
        mov messages,0
        mov sp,offset STACK
        mov si,offset command
        mov di,TBUF
        mov cx,6
        call CHAIN
        jmp fail                       ;resident dispatch must not RET
LOOPFN:
        cmp sp,offset STACK
        jne fail
        call checkstate
        inc stage
        cmp stage,5
        je passed
        jmp begin
passed:
        mov dx,offset ok
        jmp print
fail:
        push cs
        pop ds
        mov dx,offset bad
print:
        mov cx,9
        int 224
        mov cx,0
        int 224
checkstate:
        cmp refreshes,1
        jne fail
        cmp priority,200
        jne fail
        cmp user,0
        jne fail
        cmp drive,1
        jne fail
        cmp stage,4
        je legacy
        cmp keys,1
        jne fail
        cmp chains,0
        jne fail
        cmp stage,3
        je unsafe
        cmp launches,1
        jne fail
        cmp stage,1
        je launchfailed
        cmp attaches,1
        jne fail
        cmp messages,0
        jne fail
        ret
launchfailed:
        cmp attaches,0
        jne fail
        cmp messages,1
        jne fail
        ret
unsafe:
        cmp launches,0
        jne fail
        cmp messages,1
        jne fail
        ret
legacy:
        cmp launches,0
        jne fail
        cmp chains,1
        jne fail
        cmp keys,0
        jne fail
        ret
KEYOFF:
        mov keys,0
        ret
KEYON:
        mov al,CCKEYS
        mov keys,al
        ret
REFRESH:
        inc refreshes
        ret
ERMSG:
        inc messages
        pop si
skipmsg:
        lodsb
        or al,al
        jnz skipmsg
        push si
        ret
mockbdos:
        cmp cx,32
        jne md25
        cmp dx,255
        jne setuser
        mov al,user
        ret
setuser:
        mov user,dl
        ret
md25:
        cmp cx,25
        jne md14
        mov al,drive
        ret
md14:
        cmp cx,14
        jne md156
        mov drive,dl
        ret
md156:
        cmp cx,156
        jne md145
        push cs
        pop es
        mov bx,offset pd
        ret
md145:
        cmp cx,145
        jne md150
        mov priority,dl
        ret
md150:
        cmp cx,150
        jne md146
        cmp keys,0
        jne fail
        mov al,wantprio
        cmp priority,al
        jne fail
        cmp dx,offset CLICB
        jne fail
        cmp CLICB,0
        jne fail
        cmp CLICB+129,0
        jne fail
        mov si,offset command
        mov di,offset CLICB+1
        mov cx,7
        repe cmpsb
        jne fail
        inc launches
        mov user,9
        mov drive,3
        xor bx,bx
        mov bl,fail_launch
        ret
md146:
        cmp cx,146
        jne md26
        cmp keys,0
        jne fail
        inc attaches
        ret
md26:
        cmp cx,26
        jne md47
        cmp dx,TBUF
        jne fail
        ret
md47:
        cmp cx,47
        jne fail
        inc chains
        ret
'''+routine+'''
        rs 512
STACK equ $
        end
'''
# ASM86 conditional branches are short only; give each assertion a near jump.
n=0
def branch(match):
    global n
    n+=1
    return f'        je checkok{n}\n        jmp fail\ncheckok{n}:'
source=re.sub(r'        jne fail',branch,source)
with tempfile.TemporaryDirectory(prefix='filer-exec-') as folder:
    work=Path(folder); (work/'exec.a86').write_bytes(source.replace('\n','\r\n').encode())
    subprocess.run(['cpm86_asm86','exec.a86'],cwd=work,check=True)
    subprocess.run(['cpm86_gencmd','exec.h86','8080'],cwd=work,check=True,stdout=subprocess.PIPE)
    result=subprocess.run(['emu2','exec.cmd'],cwd=work,check=True,stdout=subprocess.PIPE,timeout=10)
    assert b'PASS' in result.stdout, result.stdout
    print(result.stdout.decode().strip())
