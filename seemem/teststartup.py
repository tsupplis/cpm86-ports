#!/usr/bin/env python3
"""Run the full browser with BIOS traps on concurrent systems, using emu2."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent
original = (root / 'seemem.a86').read_text()
for concurrent in (False, True):
    for mono in (False, True):
        source = original.replace('jmp     start', 'jmp     testbegin', 1)
        source = source.replace('int     224', 'call    testbdos')
        source = source.replace('int     11h', 'call    testequipment')
        source = source.replace('int     10h', 'call    testvideo')
        source = source.replace('int     16h', 'call    testkeyboard')
        source = source.rsplit('\nend', 1)[0]
        source += f'''
testbegin:
        mov ax,40h
        mov es,ax
        mov bx,10h
        mov ax,{0x30 if mono else 0x20}
        mov es:[bx],ax
        push cs
        pop es
        jmp start
testbdos:
        cmp cl,12
        jne testnotversion
        mov ax,{0x1431 if concurrent else 0x22}
        ret
testnotversion:
        cmp cl,6
        je testinput
        cmp cl,0
        je testexit
        ret
testequipment:
        {'jmp testfail' if concurrent else 'mov ax,' + str(0x30 if mono else 0x20)}
        ret
testvideo:
        {'jmp testfail' if concurrent else 'ret'}
testkeyboard:
        {'jmp testfail' if concurrent else 'jmp testinput'}
testinput:
        push es
        mov ax,{0xb000 if mono else 0xb800}
        mov es,ax
        mov bx,2
        mov al,es:[bx]
        cmp al,'0'
        jne testfail
        mov bx,10
        mov al,es:[bx]
        cmp al,':'
        jne testfail
        mov bx,3842
        mov al,es:[bx]
        cmp al,'S'
        jne testfail
        pop es
        mov ax,011bh
        ret
testexit:
        push cs
        pop ds
        mov dx,offset testok
        jmp testprint
testfail:
        push cs
        pop ds
        mov dx,offset testbad
testprint:
        mov cx,9
        int 224
        xor cx,cx
        int 224
testok db 'PASS: full SEEMEM startup, render and exit',13,10,'$'
testbad db 'FAIL: SEEMEM startup or forbidden BIOS call',13,10,'$'
        end
'''
        with tempfile.TemporaryDirectory(prefix='seemem-start-') as folder:
            work = Path(folder)
            (work / 'startup.a86').write_bytes(source.replace('\n', '\r\n').encode())
            for command in (['cpm86_asm86', 'startup.a86'],
                            ['cpm86_gencmd', 'startup.h86', '8080']):
                subprocess.run(command, cwd=work, check=True, stdout=subprocess.PIPE)
            result = subprocess.run(['emu2', 'startup.cmd'], cwd=work, check=True,
                                    stdout=subprocess.PIPE, timeout=10)
            assert b'PASS' in result.stdout, result.stdout
        print('PASS: full startup/render/exit, ' + ('concurrent' if concurrent else 'legacy')
              + ', ' + ('mono' if mono else 'colour'))
