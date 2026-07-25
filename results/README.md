# Campaign (19,28) — production result

Class: 2-connected C4-free graphs with V=19, E=28 (def = 11·18 − 7·28
= 2 in the rho2 = 11/7 theory).
Pipeline: `nauty-geng -q -f -C 19 28:28 i/8 | ./w2filter 19`, 8
workers (res/mod splitting), validated C filter (see ../README.md).
Generator: nauty 2.8.8+ds-5 (Ubuntu package), binary
`Nauty&Traces version 2.8081 (32 bits)`.

## VERDICT: WINDOW-2 CLASS EMPTY

- Graphs examined: **90,655,183,199** (sum of the WORKER DONE lines
  in the c_*.log files)
- Window-2 blocks (cycles ⊆ {3,5,6,7}, 2-degenerate): **0**
- A fortiori: zero blocks with graftable pairs.

Mathematical meaning: this confirms, at the strongest level tested
so far, the prediction of the Bundle Theorem (no def ≤ 2 block with
a graftable pair) — at (19,28) not even the block exists. Combined
with (17,25) EMPTY (previous campaign): the small-deficit front is
deserted.
Duration: ~37 h wall clock (8 workers, nice 10, shared machine).

Log label legend (frozen v1.0.0 output format, in Italian):
`visti` = graphs seen, `blocchi-w2` = window-2 blocks found,
`scartati` = input lines discarded.
