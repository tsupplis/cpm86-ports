#!/usr/bin/env python3
"""Validate FRAG against CP/M and DOS disks in private PCE sessions."""
import argparse
import hashlib
import os
from pathlib import Path
import pty
import re
import select
import shutil
import socket
import subprocess
import tempfile
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
                   ' ': 'space', '.': 'period', '*': '+shift 8 -shift'}.get(char, char)
            self.command('key ' + key)
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


def validate(work, system, fat=False):
    base = ROOT if not system.startswith('cdos') else ROOT.parent/'cpm86-hacking'
    image = system+'.img' if not system.startswith('cdos') else 'cdos-'+system[4]+'.'+system[5]+'.img'
    cfg = system+'.cfg'
    shutil.copyfile(base/image,work/'boot.img')
    (work/'ALPHA.BIN').write_bytes(b'A'*40000)
    (work/'ZETA.BIN').write_bytes(b'Z'*1024)
    if fat:
        run('mformat','-C','-i',work/'disk.img','-t','40','-h','2','-n','8','-c','2','::')
        for p in (ROOT/'frag/frag.cmd',work/'ALPHA.BIN',work/'ZETA.BIN'):
            run('mcopy','-i',work/'disk.img',p,'::'+p.name)
    else:
        run('python3',ROOT/'tools/cpm86twist.py','untwist',ROOT/'cpmbase.img',work/'data.img')
        for p in (ROOT/'frag/frag.cmd',work/'ZETA.BIN',work/'ALPHA.BIN'):
            run('cpmcp','-f','cpm86-320',work/'data.img',p,'0:')
        run('python3',ROOT/'tools/cpm86twist.py','twist',work/'data.img',work/'disk.img')
    original = (work/'disk.img').read_bytes()
    config = (base/cfg).read_text().replace('path = "rom"',f'path = "{base/"rom"}"')
    config = re.sub(r'speed\s*=\s*\d+', 'speed = 16', config)
    config = re.sub(r'disk \{[^{}]*\}', '',config)
    config = config.replace('driver = "sdl"','driver = "null"')
    config = config.replace('driver      = "sdl:lowpass=5000"','driver = "null"')
    for i,name in enumerate(('boot.img','disk.img')):
        config += f'\ndisk {{ drive={i} type="auto" file="{work/name}" }}\n'
    (work/'pce.cfg').write_text(config)
    m=Machine(work)
    try:
        m.until_screen(r'^A> *$')
        # Explicit B: while still on A: verifies the target-drive probe.
        m.type('b:frag b:*.bin\n')
        screen=m.until_screen('DOS/FAT disks are not supported' if fat else 'ALPHA')
        screen=m.until_screen(r'^A> *$')
        if fat:
            assert 'DOS/FAT disks are not supported' in screen,screen
            assert 'ALPHA' not in screen and 'Group' not in screen,screen
        else:
            assert screen.index('ALPHA') < screen.index('ZETA'),screen
            assert 'DOS/FAT disks' not in screen,screen
            entries=(work/'data.img').read_bytes()[4096:4096+2048]
            for o in range(0,len(entries),32):
                e=entries[o:o+32]
                if e[1:12] not in (b'ALPHA   BIN',b'ZETA    BIN'):
                    continue
                fields=[]
                for j in range(16,32,2):
                    fields.append(''.join(f'{b:02X}' if b else '  ' for b in e[j:j+2]))
                expected=f'{e[12]:02X} {e[15]:02X}   '+' '.join(fields)
                assert expected.rstrip() in screen,(expected,screen)
    finally:
        m.close()
    assert (work/'disk.img').read_bytes() == original,'FRAG/OS modified the test disk'
    print('PASS '+system+(' FAT rejected' if fat else ' CP/M maps verified')+'; disk unchanged',flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir',type=Path,required=True)
    parser.add_argument('--system',choices=('cpm86','ccpm86','cdos32','cdos41','cdos62'))
    args=parser.parse_args()
    work=args.work_dir.resolve()
    work.mkdir(parents=True,exist_ok=False)
    systems = [args.system] if args.system else ['cpm86','ccpm86','cdos32','cdos41']
    cases = [(system,fat) for system in systems for fat in ((False,True) if system.startswith('cdos') else (False,))]
    for system,fat in cases:
        d=work/(system+('-fat' if fat else '-cpm'))
        d.mkdir()
        print('Testing '+d.name,flush=True)
        validate(d,system,fat)

if __name__=='__main__':
    main()
