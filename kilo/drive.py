#!/usr/bin/env python3
"""Drive kilo.cmd under emu2 through a pty and report what happens."""
import os, pty, select, sys, time

def run(args, keys, wait=1.5):
    pid, fd = pty.fork()
    if pid == 0:
        os.execvp("emu2", ["emu2", "kilo.cmd"] + args)
    out = b""
    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    d = os.read(fd, 65536)
                except OSError:
                    return False
                if not d:
                    return False
                out += d
        return True
    drain(wait)
    mark = len(out)
    for k in keys:
        os.write(fd, k)
        drain(0.7)
    alive = True
    try:
        p, st = os.waitpid(pid, os.WNOHANG)
        alive = (p == 0)
    except ChildProcessError:
        alive = False
    if alive:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    return out, mark, alive

if __name__ == "__main__":
    name = sys.argv[1]
    keys = [k.encode().decode("unicode_escape").encode("latin1") for k in sys.argv[2:]]
    out, mark, alive = run([name], keys)
    print("still running after keys:", alive)
    print("bytes total:", len(out), " after first screen:", len(out) - mark)
    print("tail:", repr(out[-80:]))
