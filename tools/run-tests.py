#!/usr/bin/env python3
"""Run automated subproject suites without exposing the user's terminal."""
import argparse
import os
from pathlib import Path
import signal
import subprocess

REPO_ROOT = Path(__file__).resolve().parent.parent


def stop_suite(proc):
    """Stop the whole detached suite, including emulator/compiler children."""
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        proc.wait()


def interrupted(signum, frame):
    raise KeyboardInterrupt


def main():
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGHUP, interrupted)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--make', default='make', help='make executable')
    parser.add_argument('projects', nargs='+', help='subprojects with automated tests')
    args = parser.parse_args()
    failed = []
    for project in args.projects:
        print('== %s ==' % project, flush=True)
        # A new session removes /dev/tty access. Pipes also keep terminal-driven
        # emulators from switching modes through inherited standard streams.
        proc = subprocess.Popen(
            [args.make, '-C', str(REPO_ROOT / project), 'test'],
            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, start_new_session=True,
            text=True, errors='replace',
        )
        try:
            for line in proc.stdout:
                print(line, end='', flush=True)
            status = proc.wait()
        except KeyboardInterrupt:
            stop_suite(proc)
            return 130
        finally:
            if proc.poll() is None:
                stop_suite(proc)
            proc.stdout.close()
        if status:
            failed.append(project)
    if failed:
        print('Failed test projects: %s' % ' '.join(failed), flush=True)
        return 1
    print('All %d test projects passed' % len(args.projects), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
