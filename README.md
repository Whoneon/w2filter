# w2filter: certified exhaustive filter for prescribed cycle spectra

[![ci](https://github.com/Whoneon/w2filter/actions/workflows/ci.yml/badge.svg)](https://github.com/Whoneon/w2filter/actions/workflows/ci.yml)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21575202.svg)](https://doi.org/10.5281/zenodo.21575202)

Streaming filter for the graph6 output of `nauty-geng`: it selects
the graphs whose **cycle spectrum** (the set of lengths of simple
cycles) is contained in a prescribed set W, checks
**2-degeneracy**, and for the survivors classifies the **graftable
pairs**: pairs (u,v) such that *every* simple u–v path has length
in a prescribed set P.

Born as the artifact of the **ρ₂ = 11/7** campaign
(Erdős–Gyárfás project; default W = {3,5,6,7}, P = {1,3,4,5});
parametric for the later campaigns of the dyadic scale (ρ₃, ρ₄, …)
and for any cycle-spectrum-constrained enumeration problem.

## Prerequisites

- A C compiler (gcc or clang) and make.
- `nauty` (B. D. McKay) for graph generation: `apt install nauty`
  (Debian/Ubuntu), `brew install nauty` (macOS). The scripts call
  `nauty-geng`; if your build provides plain `geng`, add a symlink.
- Optional, verification and figures only: Python 3 with
  `networkx` (`pip install networkx`) for `make verify*`, and
  `matplotlib` to regenerate the benchmark figure.

Without nauty, `make test` falls back to the shipped datasets in
`tests/data/` for the (11,15) and (12,17) anchors and skips the
(14,20) one (too large to ship).

## Pipeline

```
graph6 (stdin)
   │
parse_g6          decode; domain guard 3 ≤ N ≤ 31
   │
quick_long        fast kill: SUFFICIENT, never necessary
   │              (DFS back-edge, depth gap ≥ L_max ⟹ cycle > L_max)
   │              cost O(V+E); eliminates 87.3% [measured, (14,20)]
   │
exact_long        EXACT verdict on the cycle spectrum
   │              (canonical minimum-edge enumeration, early exit)
   │              eliminates the remaining 12.7%
   │
is_2deg           2-degeneracy by peeling (exact, confluent)
   │
pair_graftable    survivors ONLY (cold path): pair classification
   │              with reachability pruning (complete)
   ▼
output            one line per block + list of graftable pairs
```

Every stage carries its own correctness argument: in compact form in
the source, in extended form in **`docs/proofs.md`** (back-edge via
LCA; canonicity; peeling confluence; pruning completeness; stack
bounds, i.e. the grouped-siblings lemma n(Δ−1)+1 for the path DFS
and the 2E+1 push count for the vertex DFS).

## Usage

```sh
make && make test        # anchors: (11,15)→8 (3 graftable), (12,17)→2, (14,20)→2
make verify              # comparison against the slow-but-independent checker

# rho2 campaign (default window):
nauty-geng -q -f -C 19 28:28 $i/8 | ./w2filter 19

# generic windows:
nauty-geng -q -c 12 17:17 | ./w2filter 12 --cycles 3,5,6,7,9,11 --pairset 1,3,5
```

`./w2filter --help` prints the interface; `--version` the release.
Further options: `--degeneracy K` (peeling threshold, default 2),
`--no-degeneracy` (skip that stage: REQUIRED for censuses of
classes with minimum degree ≥ 3, which are never 2-degenerate),
`--no-pairs` (skip pair classification), `--format legacy-it`
(v1.0.0 output labels).
Parallelism: geng res/mod splitting (`$i/8`); residue classes are
uneven, so on many cores use a mod much larger than the core count.
Per-stage statistics go to stderr (`STATS: …`). With non-default
windows, check whether
geng's `-f` (C₄-free) remains a valid pre-filter for your
parameters; if not, drop it: `exact_long` re-checks every cycle
length against W in either case.

## Output format

One stdout line per surviving graph:
`BLOCK V=<n> g6=<graph6> pairs=[(u,v), ...]`, then a final
`WORKER DONE: seen=<n> blocks=<n> discarded=<n>`; per-stage
statistics (`STATS: ...`) and a one-line provenance header (version
and full parameter set) go to stderr, and a progress line
`...seen N` is printed every 10⁸ graphs.

`--format legacy-it` reproduces the v1.0.0 Italian labels
byte-for-byte (`BLOCCO-W2`, `graftabili`, `visti`, `scartati`):
that is the format of the shipped campaign logs in `results/`
(legend: *visti* = seen, *blocchi* = blocks, *scartati* =
discarded, *superstiti* = survivors, *graftabili* = graftable
pairs).

## Guarantees and limits

- **Exhaustive on the stream**: no sampling, no silent caps.
- Domain: 3 ≤ N ≤ 31 (32-bit masks; explicit rejection outside).
- Stacks with proven bounds + assertions active even at -O3.
- Throughput: filter-only ~2×10⁶ graphs/s on (14,20) (gcc -O3, one
  core, pre-generated input); the full geng pipeline runs at
  ~7.6×10⁴ graphs/s per worker, generator-bound. Machine-dependent;
  `make fast` enables -march=native; measure yours with
  `python3 tests/bench_impls.py`.
- The quick test is *sufficient only*: it never discards good
  graphs; survivors go to the exact stage. Overall correctness does
  not depend on the pre-filter.
- Contractual exit codes: 2 = invalid options, 3 = at least one
  input line discarded (counted and reported on stderr).

## Validation

- `make test`: three anchor classes with census values asserted
  (counts AND pair lists: the target FAILS on any deviation).
- `make verify` / `make verify-param`: normalised-output comparison
  (block g6 AND pair lists) against the INDEPENDENT checker
  `tests/indep_check.py` (networkx: parser, cycles, degeneracy and
  paths all implemented differently from the C code). Dev
  dependency: `pip install networkx`.
- `make verify-random`: same comparison on random parameters
  (n, edge count, W, P) drawn from a printed seed; rerun any
  failure with `SEED=<seed> make verify-random`. Runs in CI, so
  every push explores a different slice of the parameter space.
- `tests/window_stats.sh`: seeded stage statistics over random
  windows (CSV, shipped as `paper/window_stats.csv`).
- `tests/bench_impls.py`: throughput of the three implementations
  on the anchor classes; source of the benchmark figure in the note
  (`paper/fig_bench.py`).
  `reference_filter.py` is a stdlib port of the same pipeline: a
  fast smoke check, not independent evidence.
- CI (`.github/workflows/ci.yml`): gcc + clang, -Werror,
  asan+ubsan, asserted anchors, independent verification, hostile
  inputs.

## Provenance and results

Campaign (19,28) (≈10¹¹ graphs, 8 workers): final logs in
`results/` (class empty: 90,655,183,199 graphs examined, zero
window-2 blocks). Full mathematical context: the campaign
manuscript (in preparation; see `paper/`).

MIT license. External dependency: `nauty` (B. D. McKay) for graph
generation.
