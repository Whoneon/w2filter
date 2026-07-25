#!/usr/bin/env python3
"""Throughput comparison of the three shipped implementations on the
anchor classes. Each class is generated once with geng; every
implementation is then timed on the pre-generated file, so generation
cost is excluded; interpreter startup (timed on empty input) is
subtracted; fast runs are repeated until the measurement is at least
MIN_SECONDS long. Output: CSV on stdout, one row per (class,
implementation).
Usage:  python3 tests/bench_impls.py > paper/impl_bench.csv
Env:    NXPY = interpreter that has networkx (default: python3).
"""
import os
import subprocess
import sys
import tempfile
import time

CLASSES = [(11, 15), (12, 17), (14, 20)]
MIN_SECONDS = 2.0
MAX_REPS = 300
PY = sys.executable
NXPY = os.environ.get("NXPY", "python3")


def run_once(cmd, infile):
    with open(infile, "rb") as f:
        t0 = time.perf_counter()
        subprocess.run(cmd, stdin=f, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=False)
        return time.perf_counter() - t0


def measure(cmd, infile, empty):
    startup = min(run_once(cmd, empty) for _ in range(3))
    total, reps = 0.0, 0
    while total < MIN_SECONDS and reps < MAX_REPS:
        total += run_once(cmd, infile)
        reps += 1
    per_pass = total / reps - startup
    return max(per_pass, 1e-9)


def main():
    import shutil
    if shutil.which("nauty-geng") is None:
        sys.exit("ERROR: nauty-geng not found "
                 "(install nauty, e.g. 'apt install nauty')")
    print("V,E,graphs,impl,gps")
    with tempfile.TemporaryDirectory() as tmp:
        empty = os.path.join(tmp, "empty")
        open(empty, "w").close()
        for v, e in CLASSES:
            f = os.path.join(tmp, f"c{v}_{e}.g6")
            with open(f, "wb") as out:
                subprocess.run(["nauty-geng", "-q", "-f", "-C",
                                str(v), f"{e}:{e}"],
                               stdout=out, check=True)
            graphs = sum(1 for _ in open(f, "rb"))
            impls = [
                ("w2filter (C)", ["./w2filter", str(v)]),
                ("stdlib port", [PY, "reference_filter.py", str(v)]),
                ("networkx checker",
                 [NXPY, "tests/indep_check.py", str(v)]),
            ]
            for name, cmd in impls:
                secs = measure(cmd, f, empty)
                print(f"{v},{e},{graphs},{name},"
                      f"{graphs / secs:.0f}", flush=True)


if __name__ == "__main__":
    main()
