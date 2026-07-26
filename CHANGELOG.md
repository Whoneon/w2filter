# Changelog

## 1.1.0 (unreleased)

- New options: `--degeneracy K` (peeling threshold, default 2),
  `--no-degeneracy` (skip the stage: required for censuses of
  classes with minimum degree >= 3, which are never 2-degenerate
  and would otherwise be discarded silently), `--no-pairs`.
- English output labels by default (`BLOCK`, `pairs`, `seen`,
  `blocks`, `discarded`, `survivors`); `--format legacy-it`
  reproduces the v1.0.0 output byte-for-byte (the format of the
  shipped campaign logs in `results/`).
- One-line provenance header on stderr: version and the full
  effective parameter set, so every worker log self-documents.
- New anchor assertions: legacy format, degeneracy and pairs flags.

## 1.0.0 (2026-07-25)

First public release.

- Four-stage certified pipeline: back-edge prefilter (sufficient,
  never necessary), exact canonical cycle enumeration, 2-degeneracy
  peeling, graftable-pair classification with reachability pruning.
  Proofs shipped in `docs/proofs.md`.
- Parametric cycle window and pair set (`--cycles`, `--pairset`),
  domain 3 <= N <= 31.
- Anchor tests with asserted counts and pair lists (`make test`);
  independent networkx cross-check of block g6 and pair lists
  (`make verify`, `make verify-param`); seeded randomized
  differential verification over the parameter space
  (`make verify-random`); seeded stage statistics over random
  windows (`tests/window_stats.sh`); throughput comparison of the
  three implementations (`tests/bench_impls.py`) with the benchmark
  figure in the note.
- Contractual exit codes: 2 invalid options, 3 at least one input
  line discarded. `--version` and `--help` (argparse help in the
  Python tools as well).
- Shipped anchor datasets (`tests/data/`): `make test` runs even
  without nauty installed, skipping only the (14,20) anchor;
  explicit nauty-geng checks with an install hint elsewhere.
- Production logs of the (19,28) campaign in `results/`
  (90,655,183,199 graphs examined, class empty).
- Hardening after pre-release adversarial review: strict option
  parsing, full graph6 line validation (length and charset),
  discarded-line counter and warning, test targets that fail on any
  deviation.
