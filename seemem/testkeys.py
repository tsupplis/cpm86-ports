#!/usr/bin/env python3
"""Execute the actual keyboard assembler with mocked OS input in emu2."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent
text=(root/"seemem.a86").read_text()
routine=text[text.index("readkey:\n"):text.index("sdump:\n")]
routine=routine.replace("int     224", "call    mockbdos").replace("int     16h", "call    mockbios")
source="""        cseg
        org 100h
        jmp begin
cckeys db 1
testkey db 0
want dw 0
ok db 'PASS: SEEMEM raw keys, GOTO ASCII/escape/backspace, legacy BIOS and register preservation',13,10,'$'
bad db 'FAIL: SEEMEM keyboard',13,10,'$'
begin:
        push cs
        pop ds
"""
for key,want in [(0xC8,0x4800),(0xD0,0x5000),(0xCB,0x4B00),(0xCD,0x4D00),
                 (0xC7,0x4700),(0xCF,0x4F00),(0xC9,0x4900),(0xD1,0x5100),
                 (27,0x011B),(8,0x0E08),(ord('G'),0x2247),(ord('g'),0x2267),
                 (ord('+'),0x4E2B),(ord('-'),0x4A2D),(ord('A'),0x0041)]:
    source+=f"        mov al,{key}\n        mov dx,{want}\n        call testone\n"
source+="""        mov cs:cckeys,0
        mov dx,01e41h
        call testone
        push cs
        pop ds
        mov dx,offset ok
        jmp print

testone:
        mov cs:testkey,al
        mov cs:want,dx
        mov bx,01111h
        mov cx,02222h
        mov dx,03333h
        mov si,04444h
        mov di,05555h
        mov ax,01234h
        mov ds,ax
        mov ax,02345h
        mov es,ax
        call readkey
        cmp ax,cs:want
        jne fail
        cmp bx,01111h
        jne fail
        cmp cx,02222h
        jne fail
        cmp dx,03333h
        jne fail
        cmp si,04444h
        jne fail
        cmp di,05555h
        jne fail
        mov ax,ds
        cmp ax,01234h
        jne fail
        mov ax,es
        cmp ax,02345h
        jne fail
        ret
fail:
        push cs
        pop ds
        mov dx,offset bad
print:
        mov cx,9
        int 224
        mov cx,0
        int 224
mockbdos:
        mov al,cs:testkey
        xor ah,ah
        jmp clobber
mockbios:
        mov ax,01e41h
clobber:
        mov bx,0
        mov cx,0
        mov dx,0
        mov si,0
        mov di,0
        push ax
        mov ax,04000h
        mov es,ax
        mov ds,ax
        pop ax
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
