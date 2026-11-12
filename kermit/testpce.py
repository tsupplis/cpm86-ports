#!/usr/bin/env python3
"""Exercise Kermit between private CP/M-86 and CCP/M-86 PCE machines."""
import argparse
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
            ready, _, _ = select.select([self.fd], [], [], 0.02)
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


def prepare(work, system, index, port, baud):
    directory = work / system
    directory.mkdir()
    shutil.copyfile(ROOT / (system + '.img'), directory / 'boot.img')
    stem = 'TEST' if index == 0 else 'BACK'
    binary = bytes(range(256)) * 4
    if index:
        binary = binary[::-1]
    (directory / (stem + '.BIN')).write_bytes(binary)
    (directory / (stem + '.TXT')).write_bytes(
        (f'{system} Kermit text transfer\r\n' * 3).encode() + b'\x1a')
    commands = [f'set baud {baud}', 'set file-type binary', 'set timer off']
    (directory / 'KERMIT.INI').write_bytes(('\r\n'.join(commands) + '\r\n\x1a').encode())
    run('python3', ROOT / 'tools/cpm86twist.py', 'untwist', ROOT / 'cpmbase.img', directory / 'data.img')
    for path in [ROOT / 'kermit/kermibm.cmd', directory / 'KERMIT.INI',
                 directory / (stem + '.BIN'), directory / (stem + '.TXT')]:
        run('cpmcp', '-f', 'cpm86-320', directory / 'data.img', path, '0:')
    run('python3', ROOT / 'tools/cpm86twist.py', 'twist', directory / 'data.img', directory / 'disk.img')
    config = (ROOT / (system + '.cfg')).read_text()
    config = config.replace('path = "rom"', f'path = "{ROOT / "rom"}"')
    config = config.replace('speed = 16', 'speed = 1')
    config = re.sub(r'disk \{[^{}]*\}', '', config)
    config = config.replace('driver = "sdl"', 'driver = "null"')
    config = config.replace('driver      = "sdl:lowpass=5000"', 'driver = "null"')
    driver = 'tcp:' + ('connect=1:host=127.0.0.1:' if index else '')
    driver += f'port={port}:telnet=0:usectl=0:log={directory / "serial.log"}'
    config = re.sub(r'serial \{[^{}]*\}',
                    'serial { uart="8250" address=0x3f8 irq=4 multichar=1 '
                    f'driver="{driver}" }}', config)
    for number, image in enumerate(('boot.img', 'disk.img')):
        config += f'\ndisk {{ drive={number} type="auto" file="{directory / image}" }}\n'
    (directory / 'pce.cfg').write_text(config)
    return directory


def clock(machines, count):
    for machine in machines:
        machine.send(f'c {count:x}')
    for machine in machines:
        machine.wait()


def validate(work, baud):
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        port = listener.getsockname()[1]
    machines = []
    try:
        for index, system in enumerate(('cpm86', 'ccpm86')):
            directory = prepare(work, system, index, port, baud)
            machines.append(Machine(directory))
        print('Booting CP/M-86 and CCP/M-86', flush=True)
        clock(machines, 0x3000000)
        vectors = [m.memory('before.bin', '0:30', 4) for m in machines]
        # Start RECEIVE first. Both systems accept hardware keyboard events.
        for m in reversed(machines):
            m.type('b:\nkermibm\n')
            m.until_screen(r'Kermit-86 B>')
        for extension in ('BIN', 'TXT'):
            if extension == 'TXT':
                for m in machines:
                    m.type('set file-type ascii\n')
                    m.command('c 100000')
            for sender in (0, 1):
                receiver = 1 - sender
                name = ('TEST' if sender == 0 else 'BACK') + '.' + extension
                print(f'Transferring {name}: {machines[sender].directory.name} -> '
                      f'{machines[receiver].directory.name}', flush=True)
                machines[receiver].type('receive\n')
                machines[receiver].command('c 100000')
                machines[sender].type('send ' + name.lower() + '\n')
                deadline = time.monotonic() + 120
                iteration = 0
                display_seen = [False, False]
                while time.monotonic() < deadline:
                    clock(machines, 0x10000)
                    iteration += 1
                    if iteration % 20 == 0:
                        screens = [m.screen() for m in machines]
                        for i, screen in enumerate(screens):
                            if 'Number of packets:' in screen:
                                assert screen.splitlines()[4].startswith('Number of packets:')
                                display_seen[i] = True
                        if all('Kermit-86 B>' in screen for screen in screens):
                            assert all(display_seen), 'Transfer display was not observed'
                            break
                else:
                    raise TimeoutError(f'{name} transfer did not finish; inspect saved logs')
                print('Completed ' + name, flush=True)
        for m in machines:
            m.type('exit\n')
            m.until_screen(r'^B>')
        for m, original in zip(machines, vectors):
            assert m.memory('after.bin', '0:30', 4) == original, 'COM1 vector was not restored'
            # Remove the startup script, then check a fresh interactive launch/exit.
            m.type('era kermit.ini\n')
            m.until_screen(r'^B>')
            m.type('kermibm\n')
            m.until_screen(r'Kermit-86 B>')
            m.type('exit\n')
            m.until_screen(r'^B>')
            assert m.memory('restart.bin', '0:30', 4) == original, 'COM1 vector changed after restart'
    finally:
        for m in machines:
            m.close()
    for index, system in enumerate(('cpm86', 'ccpm86')):
        directory = work / system
        other = work / ('ccpm86' if index == 0 else 'cpm86')
        stem = 'BACK' if index == 0 else 'TEST'
        run('python3', ROOT / 'tools/cpm86twist.py', 'untwist', directory / 'disk.img', directory / 'result.img')
        for extension in ('BIN', 'TXT'):
            name = stem + '.' + extension
            output = directory / ('RX.' + extension)
            run('cpmcp', '-f', 'cpm86-320', directory / 'result.img', '0:' + name, output)
            expected, actual = (other / name).read_bytes(), output.read_bytes()
            if extension == 'TXT':
                expected, actual = expected.split(b'\x1a')[0], actual.split(b'\x1a')[0]
            assert actual == expected, f'{system}: {name} contents differ'
            print(f'PASS {system}: received {name}, {len(actual)} bytes verified', flush=True)
    print('PASS clean exit, interrupt-vector restoration and restart on both systems', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, help='new directory in which to retain images and logs')
    parser.add_argument('--baud', type=int, choices=(300, 1200, 2400, 4800, 9600), default=2400)
    args = parser.parse_args()
    for command in ('pce-ibmpc', 'cpmcp', 'python3'):
        if not shutil.which(command):
            parser.error('Missing tool: ' + command)
    if args.work_dir:
        directory = args.work_dir.resolve()
        directory.mkdir(parents=True, exist_ok=False)
        validate(directory, args.baud)
    else:
        with tempfile.TemporaryDirectory(prefix='kermit-pce-') as temp:
            print('Temporary test directory:', temp, flush=True)
            validate(Path(temp), args.baud)


if __name__ == '__main__':
    main()
