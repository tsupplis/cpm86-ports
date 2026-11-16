#!/usr/bin/env python3
"""Validate SD listing and append behavior in private PCE sessions."""
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
                   ' ': 'space', '.': 'period', '*': '+shift 8 -shift', ',': 'comma', '[': 'LeftBracket', ']': 'RightBracket', '/': 'slash'}.get(char, char)
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


def validate(work, system):
    base = ROOT if system != 'cdos41' else ROOT.parent/'cpm86-hacking'
    boot = base/(system+'.img') if system != 'cdos41' else base/'cdos-4.1.img'
    shutil.copyfile(boot, work/'boot.img')
    (work/'alpha.txt').write_bytes(b'A'*128)
    (work/'zeta.txt').write_bytes(b'Z'*20000)
    prefix = b'Q'*16383
    (work/'sd.dir').write_bytes(prefix+b'\x1a')
    run('python3', ROOT/'tools/cpm86twist.py', 'untwist', ROOT/'cpmbase.img', work/'data.img')
    for p in [ROOT/'sd/sd.cmd'] + [work/n for n in ['alpha.txt','zeta.txt','sd.dir']]:
        run('cpmcp','-f','cpm86-320',work/'data.img',p,'0:')
    run('python3',ROOT/'tools/cpm86twist.py','twist',work/'data.img',work/'disk.img')
    config = (base/(system+'.cfg')).read_text().replace('path = "rom"',f'path = "{base/"rom"}"')
    config = re.sub(r'speed\s*=\s*\d+', 'speed = 1', config)
    config = re.sub(r'disk \{[^{}]*\}', '',config)
    config = config.replace('driver = "sdl"','driver = "null"')
    config = config.replace('driver      = "sdl:lowpass=5000"','driver = "null"')
    for i,name in enumerate(('boot.img','disk.img')):
        config += f'\ndisk {{ drive={i} type="auto" file="{work/name}" }}\n'
    (work/'pce.cfg').write_text(config)
    m = Machine(work)
    try:
        m.until_screen(r'^A> *$')
        m.type('b:\n')
        m.until_screen(r'^B> *$')
        for options in ['[fn','[fn]']:
            command = 'sd *.txt '+options
            m.type(command+'\n')
            for _ in range(80):
                m.command('c 100000')
                screen = m.screen()
                tail = screen.rsplit(command,1)[-1] if command in screen else ''
                lines = [line.rstrip() for line in tail.splitlines()[:-1] if line.strip()]
                if lines and re.fullmatch(r'B>', lines[-1]):
                    assert 'ALPHA' in tail and 'ZETA' in tail,screen
                    assert tail.index('ALPHA') < tail.index('ZETA'),screen
                    assert 'Error' not in tail,screen
                    break
            else:
                raise AssertionError(screen)
    finally:
        m.close()
    run('python3',ROOT/'tools/cpm86twist.py','untwist',work/'disk.img',work/'result.img')
    run('cpmcp','-f','cpm86-320',work/'result.img','0:sd.dir',work/'result.dir')
    data = (work/'result.dir').read_bytes().split(b'\x1a')[0]
    assert data.startswith(prefix), 'Existing listing prefix overwritten'
    added = data[len(prefix):]
    assert added.count(b'ALPHA') == 2 and added.count(b'ZETA') == 2,added
    print('PASS '+system+': both bracket forms; two appends preserve the 16383-byte prefix',flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir',type=Path,required=True)
    args=parser.parse_args()
    work=args.work_dir.resolve()
    work.mkdir(parents=True,exist_ok=False)
    for system in ['cpm86','ccpm86','cdos41']:
        directory=work/system
        directory.mkdir()
        print('Testing '+system,flush=True)
        validate(directory,system)


if __name__=='__main__':
    main()
