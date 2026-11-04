#!/usr/bin/env python3
"""Execute FILER's actual ATTRIB routine; mock only BDOS DMA/search/set calls."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent
text=(root/'fcopy.inc').read_text()
routine=text[text.index('ATTRIB:'):text.index('; crc subroutines')]
routine=re.sub(r'INT +224','CALL mockbdos',routine)
source=""" cseg
 org 100h
M equ byte ptr 0[bx]
SETDMA equ 26
SRCHF equ 17
ATTR equ 30
TBUF equ 80h
 jmp begin
HB_DMA rs 128
fcb db 2,'FILE    TXT'
 rs 24
current_dma dw 80h
found db 0
calls db 0
ok db 'PASS: FILER RO-only override preserves SYSTEM/archive/F1-F3; missing search restores DMA',13,10,'$'
bad db 'FAIL: FILER attributes',13,10,'$'
begin:
 push cs
 pop ds
"""
for index in range(4):
 for attrs in range(8):
  offset=index*32
  source+=f" mov found,{index}\n mov calls,0\n mov bx,Offset HB_DMA+{offset}\n"
  for i,value in enumerate(b'FILE    TXT',1):
   if i in [1,2,3]:value|=128
   if i==9 and attrs&1:value|=128
   if i==10 and attrs&2:value|=128
   if i==11 and attrs&4:value|=128
   source+=f" mov byte ptr {i}[bx],{value}\n"
  source+=" mov bx,Offset fcb\n call ATTRIB\n cmp current_dma,80h\n jne fail\n cmp calls,1\n jne fail\n mov bx,Offset fcb\n"
  for i,value in [(1,ord('F')|128),(2,ord('I')|128),(3,ord('L')|128),(9,ord('T')),(10,ord('X')|(128 if attrs&2 else 0)),(11,ord('T')|(128 if attrs&4 else 0))]:
   source+=f" cmp byte ptr {i}[bx],{value}\n jne fail\n"
source+=""" mov found,255
 mov calls,0
 mov bx,Offset fcb
 call ATTRIB
 cmp calls,0
 jne fail
 cmp current_dma,80h
 jne fail
 mov dx,Offset ok
 jmp report
fail:
 mov dx,Offset bad
report:
 mov ah,9
 int 21h
 mov ax,4c00h
 int 21h
mockbdos:
 cmp cl,26
 jne mocksearch
 mov current_dma,dx
 ret
mocksearch:
 cmp cl,17
 jne mockset
 mov al,found
 ret
mockset:
 cmp cl,30
 jne fail
 inc calls
 ret
"""+routine+" end\n"
# ASM86 uses explicit short conditionals; trampoline each assertion locally.
source=re.sub(r' jne fail',lambda m:' je $+5\n jmp fail',source)
with tempfile.TemporaryDirectory(prefix='filer-attrs-') as tmp:
 work=Path(tmp);(work/'attrs.a86').write_bytes(source.replace('\n','\r\n').encode())
 for cmd in [['cpm86_asm86','attrs.a86'],['cpm86_gencmd','attrs.h86','8080']]:
  subprocess.run(cmd,cwd=work,check=True,stdout=subprocess.PIPE)
 result=subprocess.run(['emu2','attrs.cmd'],cwd=work,check=True,capture_output=True,timeout=15)
 assert b'PASS' in result.stdout,result.stdout
 print(result.stdout.decode().strip())
