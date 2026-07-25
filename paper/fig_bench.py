#!/usr/bin/env python3
"""Figure for the note: throughput of the three shipped
implementations on the anchor classes. Input: impl_bench.csv,
produced by tests/bench_impls.py. Output: fig_bench.pdf plus the
speedup ratios on stdout. Dev dependency: matplotlib."""
import csv
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

IMPLS = [  # (csv name, legend label, color, marker)
    ("networkx checker", "networkx checker (library-based)",
     "#eda100", "s"),
    ("stdlib port", "stdlib port (same design, Python)",
     "#008300", "^"),
    ("w2filter (C)", "w2filter (C)", "#2a78d6", "o"),
]

rows = {}
with open(sys.argv[1] if len(sys.argv) > 1 else "impl_bench.csv") as f:
    for r in csv.DictReader(f):
        cls = f"({r['V']},{r['E']})"
        rows.setdefault(cls, {})[r["impl"]] = (
            int(r["graphs"]), float(r["gps"]))
classes = list(rows)


def fmt(v):
    if v >= 1e6:
        return f"{v / 1e6:.1f}M"
    if v >= 10000:
        return f"{v / 1000:.0f}k"
    if v >= 1000:
        return f"{v / 1000:.1f}k"
    return f"{v:.0f}"


fig, ax = plt.subplots(figsize=(5.8, 2.9))
ys = list(range(len(classes)))[::-1]
for y, cls in zip(ys, classes):
    vals = [rows[cls][name][1] for name, _, _, _ in IMPLS]
    ax.plot([min(vals), max(vals)], [y, y], color="#e1e0d9",
            lw=1.2, zorder=1)
    ratio = rows[cls]["w2filter (C)"][1] / rows[cls]["networkx checker"][1]
    ax.annotate(f"$\\times${ratio:.0f}", (max(vals), y),
                xytext=(14, -3), textcoords="offset points",
                fontsize=8.5, color="#0b0b0b")
for name, label, color, marker in IMPLS:
    xs = [rows[cls][name][1] for cls in classes]
    ax.scatter(xs, ys, s=48, c=color, marker=marker, label=label,
               zorder=3)
    for x, y in zip(xs, ys):
        ax.annotate(fmt(x), (x, y), xytext=(0, 8),
                    textcoords="offset points", ha="center",
                    fontsize=7.5, color="#52514e")
ax.set_yticks(ys)
ax.set_yticklabels([f"{cls}\n{rows[cls][IMPLS[0][0]][0]:,} graphs"
                    for cls in classes], fontsize=8.5)
ax.set_xscale("log")
ax.set_xlabel("throughput (graphs per second, log scale)")
ax.set_ylim(-0.6, len(classes) - 0.2)
ax.grid(axis="x", color="#e1e0d9", lw=0.5)
ax.set_axisbelow(True)
for s in ("top", "right", "left"):
    ax.spines[s].set_visible(False)
ax.tick_params(axis="y", length=0)
ax.legend(fontsize=8, frameon=False, loc="upper center",
          bbox_to_anchor=(0.44, 1.22), ncols=3, columnspacing=0.9,
          handletextpad=0.3)
fig.tight_layout()
fig.savefig("fig_bench.pdf")

for cls in classes:
    c = rows[cls]["w2filter (C)"][1]
    s = rows[cls]["stdlib port"][1]
    n = rows[cls]["networkx checker"][1]
    print(f"{cls}: C={fmt(c)} stdlib={fmt(s)} networkx={fmt(n)} "
          f"C/stdlib={c / s:.1f} C/networkx={c / n:.0f}")
