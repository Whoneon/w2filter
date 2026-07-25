#!/usr/bin/env python
"""INDEPENDENT checker (networkx: g6 parser, simple_cycles,
core_number, all_simple_paths: no algorithm in common with
w2filter.c or with reference_filter.py). Usage:
  ... | indep_check.py N --cycles 3,5,6,7 --pairset 1,3,4,5
Prints normalised lines:  g6 SORTED_PAIRS
for the graphs with cycle spectrum ⊆ W that are 2-degenerate.
"""
import argparse
import sys

ap = argparse.ArgumentParser(
    description="Independent networkx checker for w2filter: g6 "
                "parser, cycle enumeration, degeneracy and path "
                "enumeration all implemented differently from the C "
                "code. Reads graph6 from stdin, prints normalised "
                "lines 'g6 SORTED_PAIRS'.")
ap.add_argument("N", type=int, help="expected number of vertices")
ap.add_argument("--cycles", default="3,5,6,7",
                help="allowed cycle lengths W (default 3,5,6,7)")
ap.add_argument("--pairset", default="1,3,4,5",
                help="allowed path lengths P (default 1,3,4,5)")
args = ap.parse_args()
N = args.N
W = set(map(int, args.cycles.split(",")))
P = set(map(int, args.pairset.split(",")))

import networkx as nx  # after argparse: --help works without networkx

for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    G = nx.from_graph6_bytes(line.encode())
    assert G.number_of_nodes() == N, line
    # exact cycle spectrum
    ok = True
    for cyc in nx.simple_cycles(G):
        if len(cyc) not in W:
            ok = False
            break
    if not ok:
        continue
    if max(nx.core_number(G).values(), default=0) > 2:
        continue
    pairs = []
    for u in range(N):
        for v in range(u + 1, N):
            T = {len(p) - 1 for p in nx.all_simple_paths(G, u, v)}
            if T and T <= P:
                pairs.append((u, v))
    print(f"{line} {sorted(pairs)}", flush=True)
