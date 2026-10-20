#!/usr/bin/env python3
"""Run the COM's unchanged machine code on an 8086 with mock OS/disk calls.

Requires cc and an emu2-cpm86 source checkout (the crossdev archive is used
by default). Override EMU2_SOURCE with the path to its src/cpu.c.
No system emulator, mounted image, real disk or reset facility is invoked.
"""
from pathlib import Path
import os
import re
import shutil
import struct
import subprocess

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parent.parent / 'setboot'
WORK = TOOLS / 'work'
WORK.mkdir(exist_ok=True)
tool = shutil.which('pcdev_masm1')
default_cpu = Path(tool).resolve().parent.parent / 'archive/emu2/src/cpu.c' if tool else Path('missing')
cpu = Path(os.environ.get('EMU2_SOURCE', default_cpu))
if not cpu.is_file():
    raise SystemExit('Set EMU2_SOURCE to the emu2-cpm86 src/cpu.c path.')
subprocess.run(['cc', '-std=c11', '-O2', '-DEMU_CPU_SOURCE="' + str(cpu) + '"',
                str(TOOLS / 'runner.c'), '-o', str(WORK / 'runner')], check=True)


def mbr(entries=((0x80, 0x06, 63, 10000), (0, 0xDB, 10063, 20000),
                 (0, 0x05, 30063, 40000), (0, 0x99, 70063, 12345))):
    # Patterned bootstrap and disk signature make preservation checks useful.
    data = bytearray((i * 37 + 19) & 255 for i in range(512))
    data[446:510] = bytes(64)
    for i, (flag, typ, start, count) in enumerate(entries):
        struct.pack_into('<B3sB3sII', data, 446 + 16*i, flag, b'\x01\x01\x00',
                         typ, b'\x02\x20\x08', start, count)
    data[510:] = b'\x55\xaa'
    return data


count = 0

def run(mode, tail='', data=None, scenario='ok', text=None, writes=0, reboot=0,
        usage=False, changes=False, reads=None, locks=None, nmi=0):
    global count
    data = mbr() if data is None else data
    (WORK / 'input.mbr').write_bytes(data)
    result = subprocess.run([str(WORK / 'runner'), str(ROOT/'setboot.com'), str(mode),
                             tail, str(WORK/'input.mbr'), str(WORK/'output.mbr'), scenario],
                            capture_output=True, text=True, timeout=5)
    assert result.returncode == 0, (mode, tail, scenario, result.stdout, result.stderr)
    head, _, out = result.stdout.partition('\n')
    stats = {k: int(v) for k, v in re.findall(r'(\w+)=(\d+)', head)}
    assert stats['nmi'] == nmi, result.stdout
    assert stats['writes'] == writes, (mode, tail, scenario, result.stdout)
    assert stats['reboot'] == reboot, result.stdout
    if reads is not None:
        assert stats['reads'] == reads, result.stdout
    if locks is not None:
        assert stats['locks'] == locks, result.stdout
    if text:
        assert text in out, (mode, tail, text, result.stdout)
    assert ('Usage:' in out) == usage, result.stdout
    if usage:
        assert ('[partition] [/R]' if mode == 0 else '[partition] [R]') in out
    actual = (WORK/'output.mbr').read_bytes()
    if changes:
        wanted = bytearray(data)
        for i in range(4):
            wanted[446+16*i] = 0x80 if i == int(tail.split()[0])-1 else 0
        assert actual == wanted, 'MBR changed outside the four active flags'
    else:
        assert actual == data, 'Unexpected MBR mutation'
    count += 1
    return result.stdout


for mode in range(3):
    out = run(mode, text='Digital Research CP/M-86', reads=1)
    assert 'Extended (logical partitions omitted)' in out and 'Unknown' in out
    assert '70063' in out and '12345' in out and 'FAT16' in out
    assert out.count('Unknown') == 1
    run(mode, '2:', scenario='disk2', text='Physical disk 2', reads=1)
    run(mode, '2', text='MBR write verified', writes=1, changes=True, reads=2)
    run(mode, '4', text='MBR write verified', writes=1, changes=True)
    run(mode, '1', text='already active', reads=1)
    for r in ('/R', '/r', '[R', '[R]', '[r]'):
        run(mode, '2 '+r, text='Rebooting', writes=1, reboot=1, changes=True)
        run(mode, '1 '+r, text='already active', reboot=1)
    for tail in ('x', '0', '5', '129:', '0:', '01:', '1: 2:', '1 2', '1x', '2:3',
                 '/R', '[R', '[R]', '2 /R /R', '2 [R]]', '2 /RX', '2 /', '2 [',
                 '2 [ R]', '2 R', '1:1', '99999999999:', '1 1:', '2 /R ]', '2 /R]'):
        # 1 1: is valid: options may appear in either order.
        if tail == '1 1:':
            run(mode, tail, text='already active')
        else:
            run(mode, tail, text='Invalid parameters', usage=True, reads=0, locks=0)
    run(mode, '3', text='container', usage=True)
    for typ in (0x0F, 0x85, 0xEE, 0x42):
        data = mbr();data[446+2*16+4] = typ
        run(mode, '3', data=data, text='container', usage=True)
    run(mode, '2', scenario='readfail', text='Cannot read MBR', reads=3)
    run(mode, '2', scenario='writefail', text='write failed', writes=3)
    run(mode, '2', scenario='mismatch', text='does not match', writes=1, changes=True)
    run(mode, '2', scenario='verifyread', text='Cannot read back', writes=1, changes=True)
    for scenario in ('writefail', 'mismatch', 'verifyread'):
        run(mode, '2 /R', scenario=scenario, text='No reboot',
            writes=3 if scenario=='writefail' else 1, changes=scenario!='writefail')
    run(mode, scenario='absent', text='Cannot read MBR', reads=3)
    # Explicit disk failure gets OS-specific usage.
    run(mode, '1:', scenario='absent', text='Cannot read MBR', reads=3, usage=True)
    data = mbr();data[510:] = bytes(2)
    run(mode, data=data, text='Missing MBR signature')
    run(mode, '1:', data=data, text='Missing MBR signature', usage=True)
    run(mode, data=mbr(()), text='No nonempty primary partitions')
    run(mode, '2', data=mbr(((0x80,6,63,10000),)), text='slot is empty', usage=True)
    for data in (mbr(((0x81,6,63,10000),)),
                 mbr(((0x80,6,63,10000),(0x80,0xdb,10063,1000))),
                 mbr(((0x80,6,63,10000),(0,0xdb,500,20000))),
                 mbr(((0x80,6,63,10000),(0,0xdb,0,100))),
                 mbr(((0x80,6,63,10000),(0,0xdb,10063,0))),
                 mbr(((0x80,6,63,10000),(0,0xdb,0xfffffffe,10)))):
        run(mode, '2', data=data, text='Invalid primary table')
    # Large unsigned fields exercise 32-bit decimal on the 8086.
    run(mode, data=mbr(((0,0x83,0x80000000,0xffffffff),)), text='2147483648  4294967295')
    run(mode, '2', scenario='boundary', text='MBR write verified', writes=1, changes=True)
    run(mode, ' ' * 127, text='Physical disk 1', reads=1)
    run(mode, '\t2\t1:\t', text='MBR write verified', writes=1, changes=True)

for scenario in ('busy', 'race', 'consolebusy', 'remote', 'queueopen', 'queueread'):
    run(2, '2 /R', scenario=scenario, reads=0,
        text='MXdisk' if scenario.startswith('queue') else 'local console')
run(2, '2 /R', scenario='queuerelease', writes=1, changes=True, text='Cannot release')
for mode in (0, 1):
    run(mode, '128:', scenario='disk128', text='Physical disk 128')
run(2, '3:', text='does not exist', usage=True, reads=0)
# Wrapping end rejected, but adjacent entries and unsorted disjoint entries work.
for mode in range(3):
    run(mode, '2', data=mbr(((0x80,6,50000,100),(0,0xdb,63,1000))),
        text='MBR write verified', writes=1, changes=True)

# Entry-register and prefix checks must run before DS normalization or OS calls.
for mode in range(3):
    run(mode, scenario='bad_ds', reads=0, locks=0, nmi=1)
    run(mode, scenario='other_es', text='Physical disk 1', reads=1)
for mode in (1, 2):
    for scenario in ('bad_model', 'bad_code', 'null_e0'):
        run(mode, scenario=scenario, reads=0, locks=0, nmi=1)
run(0, scenario='bad_prefix', reads=0, locks=0, nmi=1)
run(0, scenario='other_call5', text='Physical disk 1', reads=1)

# CMD has the exact same payload at +128+256, padded to 128-byte records.
com = (ROOT/'setboot.com').read_bytes()
cmd = (ROOT/'setboot.cmd').read_bytes()
assert cmd[0] == 1 and cmd[9] == 0
assert cmd[128:384] == bytes(256)
assert cmd[384:384+len(com)] == com
assert not any(cmd[384+len(com):])
print(f'{count} synthetic 8086 execution cases passed; COM/CMD payloads match.')
