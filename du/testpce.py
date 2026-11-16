#!/usr/bin/env python3
"""Validate DU BIOS/XIOS access using disposable PCE disk images."""
import argparse
import os
from pathlib import Path
import pty
import re
import select
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent


def run(*args):
    subprocess.run([str(x) for x in args], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)


class Machine:
    def __init__(self, directory):
        self.directory = directory
        self.log = (directory / 'monitor.log').open('wb')
        self.fd, slave = pty.openpty()
        self.process = subprocess.Popen(
            ['pce-ibmpc', '-c', str(directory / 'pce.cfg'), '-t', 'null', '-g', 'vga'],
            cwd=directory, stdin=slave, stdout=slave, stderr=slave)
        os.close(slave)
        try:
            self.wait()
        except Exception:
            self.close()
            raise

    def send(self, command):
        os.write(self.fd, command.encode() + b'\n')

    def wait(self):
        result = bytearray()
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.fd], [], [], 0.001)
            if ready:
                block = os.read(self.fd, 65536)
                if not block:
                    raise RuntimeError('PCE closed its monitor')
                self.log.write(block)
                self.log.flush()
                result.extend(block)
            elif result.endswith(b'\n-'):
                return bytes(result)
            if self.process.poll() is not None:
                raise RuntimeError('PCE exited unexpectedly')
        raise TimeoutError('PCE monitor timeout: ' + str(self.directory))

    def command(self, command):
        self.send(command)
        return self.wait()

    def type(self, text):
        for char in text:
            key = {':': '+shift semicolon -shift', '\n': 'return',
                   ' ': 'space', '.': 'period', '*': '+shift 8 -shift', '!': '+shift 1 -shift', ',': 'comma', '[': 'LeftBracket', ']': 'RightBracket', '/': 'slash'}.get(char, char)
            result = self.command('key ' + key)
            if b'unknown key' in result:
                raise AssertionError(result)
            self.command('c 10000')

    def memory(self, name, address, size):
        path = self.directory / name
        self.command(f'save {path} bin {address} {size:x}')
        return path.read_bytes()

    def screen(self):
        data = self.memory('screen.bin', 'b800:0', 4000)[::2]
        text = '\n'.join(data[i:i + 80].decode('cp437') for i in range(0, 2000, 80))
        (self.directory / 'screen.txt').write_text(text)
        return text

    def until_screen(self, pattern):
        for _ in range(80):
            self.command('c 100000')
            screen = self.screen()
            if re.search(pattern, screen, re.M):
                return screen
        raise TimeoutError('Expected screen prompt: ' + pattern)

    def close(self):
        try:
            if self.process.poll() is None:
                self.send('q')
                deadline = time.monotonic() + 5
                while self.process.poll() is None and time.monotonic() < deadline:
                    ready, _, _ = select.select([self.fd], [], [], 0.05)
                    if ready:
                        try:
                            self.log.write(os.read(self.fd, 65536))
                        except OSError:
                            break
                if self.process.poll() is None:
                    self.process.terminate()
                    try:
                        self.process.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        self.process.kill()
                self.process.wait(timeout=5)
        finally:
            os.close(self.fd)
            self.log.close()


def validate(work, system, fat=False, probe=False):
    base = ROOT if system != 'cdos41' else ROOT.parent/'cpm86-hacking'
    image = system+'.img' if system != 'cdos41' else 'cdos-4.1.img'
    shutil.copyfile(base/image,work/'boot.img')
    # A diagnostic companion reports the real BDOS version independently of DU.
    (work/'ver.a86').write_bytes("""CSEG
ORG 100H
MOV CL,12
INT 224
MOV BX,AX
MOV CX,4
NEXT:
PUSH CX
MOV CL,4
ROL BX,CL
MOV DL,BL
AND DL,15
ADD DL,'0'
CMP DL,'9'
JBE DIGIT
ADD DL,7
DIGIT:
PUSH BX
MOV CL,2
INT 224
POP BX
POP CX
LOOP NEXT
MOV CL,0
XOR DL,DL
INT 224
END
""".replace('\n','\r\n').encode())
    subprocess.run(['cpm86_asm86','ver.a86'],cwd=work,check=True,stdout=subprocess.DEVNULL)
    subprocess.run(['cpm86_gencmd','ver.h86','8080'],cwd=work,check=True,stdout=subprocess.DEVNULL)
    if fat:
        run('mformat','-C','-i',work/'disk.img','-t','40','-h','2','-n','8','-c','2','::')
        run('mcopy','-i',work/'disk.img',ROOT/'du/du.cmd','::du.cmd')
        run('mcopy','-i',work/'disk.img',work/'ver.cmd','::ver.cmd')
    else:
        run('python3',ROOT/'tools/cpm86twist.py','untwist',ROOT/'cpmbase.img',work/'data.img')
        run('cpmcp','-f','cpm86-320',work/'data.img',ROOT/'du/du.cmd','0:')
        run('cpmcp','-f','cpm86-320',work/'data.img',work/'ver.cmd','0:')
        data=bytearray((work/'data.img').read_bytes())
        # Track 30 is outside the small fixture's allocated directory/file area.
        for record in range(32):
            offset=30*4096+record*128
            data[offset:offset+128]=(f'DUREC{record:02d} '.encode()*16)
        (work/'data.img').write_bytes(data)
        run('python3',ROOT/'tools/cpm86twist.py','twist',work/'data.img',work/'disk.img')
    original=(work/'disk.img').read_bytes()
    config=(base/(system+'.cfg')).read_text().replace('path = "rom"',f'path = "{base/"rom"}"')
    config=re.sub(r'speed\s*=\s*\d+', 'speed = 16', config)
    config=re.sub(r'disk \{[^{}]*\}', '',config)
    config=config.replace('driver = "sdl"','driver = "null"').replace('driver      = "sdl:lowpass=5000"','driver = "null"')
    for i,name in enumerate(('boot.img','disk.img')):
        config+=f'\ndisk {{ drive={i} type="auto" file="{work/name}" }}\n'
    (work/'pce.cfg').write_text(config)
    m=Machine(work)
    def command(text):
        m.type(text+'\n')
        screen=m.until_screen(r'^DU B: *$')
        assert not re.search(r'failed|Cannot|CANNOT|Unsupported|UNEXPECTED|^\? *$',screen,re.M),screen
        return screen
    try:
        m.until_screen(r'^A> *$')
        m.type('b:\n')
        m.until_screen(r'^B> *$')
        m.type('ver\n')
        screen=m.until_screen(r'^[0-9A-F]{4}')
        print('BDOS version: '+re.search(r'^([0-9A-F]{4})',screen,re.M)[1],flush=True)
        m.until_screen(r'^B> *$')
        if probe:
            return
        m.type('du\n')
        if fat:
            screen=m.until_screen('DOS-format disks are not supported')
            m.until_screen(r'^B> *$')
        else:
            m.until_screen(r'^DU B: *$')
            screen=command('t30!s1!r!a')
            assert 'DUREC00' in screen,screen
            screen=command('s2!r!a')
            assert 'DUREC01' in screen,screen
            command('ch0,5a!w')
            # Switch physical sector, then return: force a disk reread.
            command('s5!r')
            screen=command('s2!r!a')
            assert 'ZUREC01' in screen,screen
            m.type('x\n')
            m.until_screen('Confirm quiting')
            m.type('y')
            m.until_screen(r'^B> *$')
    finally:
        m.close()
    after=(work/'disk.img').read_bytes()
    expected=bytearray(original)
    if not fat:
        # Track 30 -> cylinder 30/head 0, second 128-byte record.
        expected[30*8192+128]=ord('Z')
    assert after==expected, 'Unexpected disk changes: '+str([(i,a,b) for i,(a,b) in enumerate(zip(expected,after)) if a!=b][:20])
    print('PASS '+system+(' DOS disk rejected, unchanged' if fat else ' read/write/reread, whole disk verified'),flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir',type=Path,required=True)
    parser.add_argument('--system',choices=('cpm86','ccpm86','cdos41'))
    parser.add_argument('--probe-only',action='store_true')
    args=parser.parse_args()
    import hashlib
    print('Production du.cmd SHA256 '+hashlib.sha256((ROOT/'du/du.cmd').read_bytes()).hexdigest(),flush=True)
    work=args.work_dir.resolve()
    work.mkdir(parents=True,exist_ok=False)
    for system in ([args.system] if args.system else ['cpm86','ccpm86','cdos41']):
        for fat in ([False,True] if system=='cdos41' else [False]):
            directory=work/(system+('-fat' if fat else '-cpm'))
            directory.mkdir()
            print('Testing '+directory.name,flush=True)
            validate(directory,system,fat,args.probe_only)


if __name__=='__main__':
    main()
