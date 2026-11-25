#!/usr/bin/env python3
"""Run fefw with --max-seconds and check that its 124 exit is the deadline exit.

fefw ends a bounded run on purpose: when `--max-seconds` expires, its deadline
thread logs the state and calls `TerminateProcess(GetCurrentProcess(), 124)`.
GNU `timeout` also uses 124 for its own timeout, so a wrapper that checks only
the status cannot tell the intended deadline exit from an overrun.

This check starts the runtime itself, measures wall time around that child and
waits only until `--max-seconds` plus `--grace-seconds`. It passes when the
child exits 124 inside that window, and reports an overrun only when the child
is still running at the limit and has to be killed.

    python tools/check_fefw_deadline.py --exe X:/fefw-recomp-0.2.13/fefw.exe \
        --max-seconds 25 --ini X:/fefw-recomp-0.2.13/fefw.ini --headless

A clean exit 0 before the deadline also passes, with a note that the deadline
was not exercised; any other status is a failure.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path
from typing import NamedTuple


class Verdict(NamedTuple):
    status: str
    ok: bool
    detail: str


def classify(returncode: int | None, max_seconds: int, grace_seconds: float) -> Verdict:
    """Map a finished run to a verdict. `returncode` is None when it was killed."""
    if returncode is None:
        return Verdict(
            "overrun",
            False,
            f"still running at the {max_seconds + grace_seconds:g}s limit; killed",
        )
    if returncode == 124:
        return Verdict(
            "deadline",
            True,
            f"exit 124 at the configured {max_seconds}s deadline",
        )
    if returncode == 0:
        return Verdict(
            "clean",
            True,
            "exit 0 before the deadline; the deadline was not exercised",
        )
    return Verdict("failed", False, f"exit {returncode} before the deadline")


def build_command(args: argparse.Namespace) -> list[str]:
    command = [str(args.exe), "--max-seconds", str(args.max_seconds)]
    if args.headless:
        command.append("--headless")
    if args.ini:
        command += ["--ini", str(args.ini)]
    if args.log:
        command += ["--log", str(args.log)]
    command += list(args.extra)
    return command


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--exe", type=Path, default=Path("fefw.exe"),
                        help="fefw.exe to run (default: ./fefw.exe)")
    parser.add_argument("--max-seconds", type=int, required=True,
                        help="the run's --max-seconds value")
    parser.add_argument("--grace-seconds", type=float, default=15.0,
                        help="wall time allowed past the deadline for startup and teardown "
                             "before the run counts as an overrun (default: 15, the runtime's "
                             "own teardown bound)")
    parser.add_argument("--ini", type=Path, help="pass --ini FILE")
    parser.add_argument("--log", type=Path, help="pass --log FILE")
    parser.add_argument("--headless", action="store_true", help="pass --headless")
    parser.add_argument("--cwd", type=Path, help="working directory for the run")
    parser.add_argument("extra", nargs=argparse.REMAINDER, metavar="--",
                        help="remaining arguments are passed to fefw unchanged")
    args = parser.parse_args(argv)
    if args.max_seconds <= 0:
        parser.error("--max-seconds must be positive; without a deadline there is no 124 to expect")
    if args.grace_seconds < 0:
        parser.error("--grace-seconds cannot be negative")

    command = build_command(args)
    limit = args.max_seconds + args.grace_seconds
    start = time.monotonic()
    process = subprocess.Popen(command, cwd=args.cwd)
    try:
        returncode = process.wait(timeout=limit)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        returncode = None
    elapsed = time.monotonic() - start

    verdict = classify(returncode, args.max_seconds, args.grace_seconds)
    print(f"{'PASS' if verdict.ok else 'FAIL'} {verdict.status}: {verdict.detail}; "
          f"wall {elapsed:.2f}s (limit {limit:g}s)")
    return 0 if verdict.ok else 1


if __name__ == "__main__":
    sys.exit(main())
