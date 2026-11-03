#!/usr/bin/env python3
"""Disposable FAT browser regression. Explicit boot/config; no production writes.

Example: python3 tools/hidden_browser_check.py wash15 --boot BOOT.img --config PCE.cfg
Run one invocation at a time. Requires PCE, mtools and a CDOS kernel with the
HIDDEN deletion fix. The supplied boot disk and configuration are copied/read,
never modified. Each test terminates its own emulator with q or timeout.
"""
from pathlib import Path
import argparse, os, re, subprocess, sys, tempfile
R=Path(__file__).resolve().parent.parent

def run(d, *args):
    result = subprocess.run(args, cwd=d, capture_output=True)
    if result.returncode:
        raise RuntimeError((args, result.stderr.decode(errors="replace")))
    return result.stdout


def entries(image):
    b = image.read_bytes()
    sector = int.from_bytes(b[11:13], "little")
    start = (
        int.from_bytes(b[14:16], "little") + b[16] * int.from_bytes(b[22:24], "little")
    ) * sector
    return {
        b[o : o + 11]: b[o : o + 32]
        for o in range(start, start + int.from_bytes(b[17:19], "little") * 32, 32)
        if b[o] not in [0, 229]
    }


def fixture(prefix):
    d = Path(tempfile.mkdtemp(prefix=prefix))
    print("Diagnostics:", d, flush=True)
    (d / "cdos86.img").write_bytes(BOOT.read_bytes())
    run(
        d,
        "mformat",
        "-C",
        "-i",
        "fat.img",
        "-t",
        "40",
        "-h",
        "2",
        "-n",
        "8",
        "-c",
        "2",
        "::",
    )
    (d / "data").write_bytes(b"KEEP CONTENT\r\n\x1a")
    for name, attribute in [
        ("N.TXT", ""),
        ("H.TXT", "+h"),
        ("S.TXT", "+s"),
        ("SH.TXT", "+s +h"),
        ("RO.TXT", "+r"),
        ("HR.TXT", "+h +r"),
        ("Q.TXT", "+h"),
        ("W1.DAT", "+h"),
        ("W2.DAT", "+s"),
    ]:
        run(d, "mcopy", "-i", "fat.img", "data", "::" + name)
        if attribute:
            run(d, "mattrib", "-i", "fat.img", *attribute.split(), "::" + name)
    run(d, "mmd", "-i", "fat.img", "::SUB")
    run(d, "mlabel", "-i", "fat.img", "::KEEPVOL")
    return d


def session(d, cases):
    cfg = (
        CONFIG
        .read_text()
        .replace('driver = "sdl"', 'driver = "null"')
        .replace('driver      = "sdl:lowpass=5000"', 'driver      = "null"')
        .replace('"cdostools.img"', '"fat.img"')
        .replace('"ccpmtools.img"', '"fat.img"')
        .replace('"ccpm86.img"', '"cdos86.img"')
    )
    cfg = re.sub(
        r'(?m)^path\s*=\s*"([^"\n]+)"',
        lambda m: 'path = "' + str((CONFIG.parent / m[1]).resolve()) + '"',
        cfg,
    )
    (d / "cfg").write_text(cfg)
    keys = {
        " ": "Space",
        "\r": "Return",
        "\x04": "+Ctrl d -Ctrl",
        ":": "+Shift Semicolon -Shift",
        ".": "Period",
        "[": "LeftBracket",
        "]": "RightBracket",
        "=": "Equal",
        "*": "+Shift 8 -Shift",
        ",": "Comma",
        "-": "Minus",
        "/": "Slash",
        "\\": "Backslash",
    }
    commands = ["c f000000"]

    def text(value):
        for ch in value:
            commands.extend(["key " + keys.get(ch, ch), "c 200000"])

    for command, label, answer in cases:
        text("cls")
        commands.extend(["key Return", "c 2000000"])
        text(command)
        commands.extend(["key Return", "c 4000000"])
        if answer:
            if args.tool == 'filer':
                # Output polling consumes keys during redraw; pace actions.
                for chunk in answer.split('|'):
                    text(chunk)
                    commands.append('c 20000000')
            else:
                text(answer)
            commands.append("c 4000000")
        commands.append(f'save "{label}.bin" bin b800:0000 fa0')
    commands.append("q")
    with (d / "pce.log").open("wb") as log:
        subprocess.run(
            ["pce-ibmpc", "-c", "cfg", "-t", "null", "-g", "vga"],
            cwd=d,
            input=("\n".join(commands) + "\n").encode(),
            stdout=log,
            stderr=log,
            check=True,
            timeout=180,
        )
    screens = {}
    for _, label, _ in cases:
        b = (d / (label + ".bin")).read_bytes()
        screens[label] = "\n".join(
            b[o : o + 160 : 2].decode("ascii", "replace").rstrip()
            for o in range(0, 3840, 160)
        )
        (d / (label + ".txt")).write_text(screens[label])
    return screens




def classic_check():
    """Original kernels: separate CP/M data disk with a SYS+RO file."""
    d=Path(tempfile.mkdtemp(prefix='browser-classic-'))
    print('Diagnostics:',d,flush=True)
    (d/'diskdefs').write_bytes((R/'diskdefs').read_bytes())
    def twist(mode,source,target):
        run(d,'python3',str(R/'tools/cpm86twist.py'),mode,str(source),str(target))
    (d/'cdos86.img').write_bytes(BOOT.read_bytes())
    twist('untwist',args.blank.resolve(),d/'data.logical')
    run(d,'cpmcp','-f','cpm86-320','data.logical',str(R/args.tool/(args.tool+'.cmd')),'0:BROWSE.CMD')
    (d/'text').write_bytes(b'UNCHANGED\r\n\x1a')
    run(d,'cpmcp','-f','cpm86-320','data.logical','text','0:SYSTEM.TXT')
    b=bytearray((d/'data.logical').read_bytes())
    for o in range(4096,4096+64*32,32):
        if b[o+1:o+12]==b'SYSTEM  TXT': b[o+9]|=128; b[o+10]|=128
    (d/'data.logical').write_bytes(b)
    twist('twist',d/'data.logical',d/'fat.img')
    if args.tool=='filer':
        screen=session(d,[('b:browse b','classic','o| |d|y')])['classic']
        twist('untwist',d/'fat.img',d/'after.logical')
        assert b'system.txt' not in run(d,'cpmls','-f','cpm86-320','after.logical').lower(),screen
    else:
        screen=session(d,[('b:browse b:system.txt','classic','ox')])['classic']
        assert 'No matching normal files' in screen and re.search(r'SYSTEM +[.]TXT *S',screen),screen
        assert '.TXT SH' not in screen and '.TXT  H' not in screen,screen
    print('PASS: '+args.tool+' original-kernel SYSTEM/RO compatibility: '+str(BOOT),flush=True)

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tool',choices=['disk7','wash15','filer'])
    parser.add_argument('--boot',type=Path,required=True)
    parser.add_argument('--config',type=Path,required=True)
    parser.add_argument('--classic',action='store_true',help='CP/M data disk; FILER also checks its RO override')
    parser.add_argument('--blank',type=Path,default=R/'cpmbase.img',help='blank physical CP/M 320K template')
    args=parser.parse_args()
    BOOT=args.boot.resolve(); CONFIG=args.config.resolve()
    if args.classic:
        classic_check()
        sys.exit(0)
    h=sys.modules[__name__]
    which=args.tool;d=h.fixture(which+'-hidden-operations-')
    h.run(d,'mcopy','-i','fat.img',str(R/which/(which+".cmd")),'::BROWSE.CMD')
    before=h.entries(d/'fat.img')
    if which=='filer':
        a=h.session(d,[('b:browse b','edit','o| |r|NEW.TXT\r|d|y|q|y')])
    else:
        edit='orNEW.TXT\rb\x04x' if which=='wash15' else 'otbubrNEW.TXT\rdyx'
        a=h.session(d,[('b:browse b:*.txt','normal','x'),('b:browse b:h.txt','empty','ox'),('b:browse b:*.txt','edit',edit)])
        assert 'N       .TXT' in a['normal'] and 'H       .TXT' not in a['normal'],a
        assert 'No matching normal files' in a['empty'] and 'H       .TXT' in a['empty'],a
    end=h.entries(d/'fat.img')
    assert b'H       TXT' not in end and b'NEW     TXT' not in end,end
    for n in [b'N       TXT',b'HR      TXT',b'RO      TXT',b'SH      TXT',b'KEEPVOL    ',b'SUB        ']:assert before[n]==end[n],(n,before[n],end[n])
    print('PASS: '+which+' FAT normal/all/empty toggle, sorted flags, rename, delete and preservation')
