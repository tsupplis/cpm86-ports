#!/usr/bin/env python3
"""Execute the actual keyboard assembler with mocked OS input in emu2."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent
text=(root/"filer.a86").read_text()
routine=text[text.index("CIN:   "):text.index("CINRAW:")]
routine=routine.replace("INT     224", "CALL    mockbdos")
source="""        cseg
        org 100h
M equ byte ptr 0[bx]
ESC equ 27
        jmp begin
CCKEYS db 1
CHR_LO db 0
CHR_HI db 0
inputkey db 0
secondkey db 0
want dw 0
version dw 01431h
used db 0
lastchar db 0
ESCTAB db 'A','H','B','P','C','M','D','K',0,0
ok db 'PASS: FILER raw arrows/keypad, VT52 compatibility, version gate and expansion handoff',13,10,'$'
bad db 'FAIL: FILER keyboard',13,10,'$'
begin:
        push cs
        pop ds
"""
for key in [0xC7,0xC8,0xC9,0xCB,0xCD,0xCF,0xD0,0xD1,0xD2,0xD3]:
    source+=f"        mov al,{key}\n        mov dx,{(key & 127)<<8}\n        call testone\n"
source+="""        mov CCKEYS,0
        mov secondkey,'A'
        mov al,27
        mov dx,04800h
        call testone
        mov secondkey,0
        mov al,'q'
        mov dx,'q'
        call testone
        mov used,0
        call KEYINIT
        cmp CCKEYS,1
        jne fail
        cmp lastchar,'6'
        jne fail
        call KEYOFF
        cmp lastchar,'7'
        jne fail
        call KEYON
        cmp lastchar,'6'
        jne fail
        mov CCKEYS,0
        mov version,01022h
        mov used,0
        call KEYINIT
        cmp CCKEYS,0
        jne fail
        cmp used,0
        jne fail
        mov version,01041h
        call KEYINIT
        cmp CCKEYS,0
        jne fail
        mov dx,offset ok
        jmp print
fail:
        mov dx,offset bad
print:
        mov cx,9
        int 224
        mov cx,0
        int 224

testone:
        mov inputkey,al
        mov want,dx
        call CIN
        cmp ax,want
        jne fail
        mov al,CHR_LO
        mov ah,CHR_HI
        cmp ax,want
        jne fail
        ret
CINRAW:
        mov al,inputkey
        mov ah,secondkey
        mov inputkey,ah
        ret
TTYPE:
        inc used
        mov lastchar,al
        ret
mockbdos:
        mov ax,version
        ret
"""+routine+"        end\n"

with tempfile.TemporaryDirectory(prefix="asm-keys-") as folder:
    work=Path(folder)
    (work/"keys.a86").write_bytes(source.replace("\n","\r\n").encode())
    subprocess.run(["cpm86_asm86","keys.a86"],cwd=work,check=True,stdout=subprocess.PIPE)
    subprocess.run(["cpm86_gencmd","keys.h86","8080"],cwd=work,check=True,stdout=subprocess.PIPE)
    result=subprocess.run(["emu2","keys.cmd"],cwd=work,check=True,stdout=subprocess.PIPE,timeout=10)
    assert b"PASS" in result.stdout, result.stdout
    print(result.stdout.decode().strip())
