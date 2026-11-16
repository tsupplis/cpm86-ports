#!/usr/bin/env python3
"""Validate MODEM9 between private PCE machines; run manually, outside make."""
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
                   ' ': 'space', '.': 'period'}.get(char, char)
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


def prepare(work, system, index, port):
    directory = work / system
    directory.mkdir()
    base = ROOT if system in ('cpm86', 'ccpm86') else ROOT.parent / 'cpm86-hacking'
    image = system + '.img' if system in ('cpm86', 'ccpm86') else 'cdos-4.1.img'
    cfg = system + '.cfg' if system in ('cpm86', 'ccpm86') else 'cdos41.cfg'
    shutil.copyfile(base / image, directory / 'boot.img')
    name = 'TEST.BIN' if index == 0 else 'BACK.BIN'
    data = bytes(range(256)) * 4
    if index:
        data = data[::-1]
    (directory / name).write_bytes(data)
    run('python3', ROOT / 'tools/cpm86twist.py', 'untwist', ROOT / 'cpmbase.img', directory / 'data.img')
    for path in (ROOT / 'modem9/modem9.cmd', directory / name):
        run('cpmcp', '-f', 'cpm86-320', directory / 'data.img', path, '0:')
    run('python3', ROOT / 'tools/cpm86twist.py', 'twist', directory / 'data.img', directory / 'disk.img')
    config = (base / cfg).read_text()
    config = config.replace('path = "rom"', f'path = "{base / "rom"}"')
    config = re.sub(r'speed\s*=\s*\d+', 'speed = 1', config)
    config = re.sub(r'disk \{[^{}]*\}', '', config)
    config = config.replace('driver = "sdl"', 'driver = "null"')
    config = config.replace('driver      = "sdl:lowpass=5000"', 'driver = "null"')
    driver = 'tcp:' + ('connect=1:host=127.0.0.1:' if index else '')
    driver += f'port={port}:telnet=0:usectl=0:log={directory / "serial.log"}'
    # Keep one UART: MODEM9 is compiled for COM2, not Kermit's COM1.
    config = re.sub(r'serial \{[^{}]*\}', '', config)
    config += f'\nserial {{ uart="8250" address=0x2f8 irq=3 multichar=1 driver="{driver}" }}\n'
    for number, image in enumerate(('boot.img', 'disk.img')):
        config += f'\ndisk {{ drive={number} type="auto" file="{directory / image}" }}\n'
    (directory / 'pce.cfg').write_text(config)
    return directory


def clock(machines, count):
    for m in machines:
        m.send(f'c {count:x}')
    for m in machines:
        m.wait()


def validate(work, systems):
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        port = listener.getsockname()[1]
    (work / 'binary.sha256').write_text(hashlib.sha256((ROOT / 'modem9/modem9.cmd').read_bytes()).hexdigest() + '  modem9.cmd\n')
    machines = []
    try:
        for index, system in enumerate(systems):
            machines.append(Machine(prepare(work, system, index, port)))
        print('Booting ' + ', '.join(systems), flush=True)
        clock(machines, 0x3000000)
        for m in machines:
            m.until_screen(r'^A>')
            m.type('b:\n')
            m.until_screen(r'^B>')
            m.type('modem9\n')
            m.until_screen(r'COMMAND \(H for Help\):')
            print('READY ' + m.directory.name, flush=True)
        for sender in (0,1):
            receiver = 1-sender
            name = 'TEST.BIN' if sender == 0 else 'BACK.BIN'
            print(f'Transfer {systems[sender]} -> {systems[receiver]}: {name}', flush=True)
            machines[receiver].type('r ' + name.lower() + '\n')
            machines[receiver].command('c 10000')
            machines[sender].type('s ' + name.lower() + '\n')
            deadline = time.monotonic() + 120
            for iteration in range(20000):
                if time.monotonic() > deadline:
                    raise TimeoutError('transfer timeout; inspect screen/serial logs')
                clock(machines, 0x1000)
                if iteration % 20 == 0:
                    screens = [m.screen() for m in machines]
                    if all(re.search(r'^COMMAND \(H for Help\): *$', s, re.M) for s in screens):
                        print('Completed ' + name, flush=True)
                        break
                    if iteration % 200 == 0:
                        for s in screens:
                            print(s, flush=True)
            else:
                raise TimeoutError('transfer cycle budget exhausted')
        for m in machines:
            m.type('cpm\n')
            m.until_screen(r'^B>')
    finally:
        for m in machines:
            m.close()
    for index, system in enumerate(systems):
        directory = work/system
        other = work/systems[1-index]
        name = 'BACK.BIN' if index == 0 else 'TEST.BIN'
        run('python3', ROOT/'tools/cpm86twist.py', 'untwist', directory/'disk.img', directory/'result.img')
        output = directory/'received.bin'
        run('cpmcp', '-f', 'cpm86-320', directory/'result.img', '0:'+name, output)
        assert output.read_bytes() == (other/name).read_bytes(), system + ': received contents differ'
        print('PASS ' + system + ': exact received bytes', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--peer', choices=('ccpm86','cdos41'), default='ccpm86')
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=False)
    validate(work, ('cpm86', args.peer))

if __name__ == '__main__':
    main()
