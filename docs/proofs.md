# Correctness proofs - w2filter

References: the functions cited here are in `w2filter.c`. Throughout,
W = set of allowed cycle lengths (default {3,5,6,7}), L_max = max W;
P = set of allowed (graftable) path lengths (default {1,3,4,5}),
P_max = max P.

## 1. Back-edge test (`quick_long`) - SUFFICIENT, never necessary

**Claim.** If the DFS finds a non-tree edge (v,w) with
depth(v) − depth(w) ≥ L_max, the graph contains a simple cycle of
length > L_max (hence ∉ W).

**Proof.** Let a = LCA(v,w) in the DFS tree. The two tree branches
v→a and w→a share only a, so
v →(tree) a →(tree) w →(edge) v is a simple cycle of length
(depth v − depth a) + (depth w − depth a) + 1 ≥ (depth v − depth w) + 1
≥ L_max + 1, using depth(a) ≤ depth(w). ∎

The test also holds for the iterative DFS (where w need not be an
ancestor of v): the argument only uses the LCA. The test is NOT
necessary: graphs that pass it go on to the exact test. Role:
pre-filter (in the (14,20) campaign it eliminates 87.3% of the
graphs).

## 2. Canonical cycle enumeration (`exact_long`) - EXACT

**Claim.** The function returns "kill" iff a simple cycle of length
∉ W exists.

**Proof.** (Completeness) Every simple cycle C has a unique
minimum-index edge s = {a,b}; C corresponds bijectively to a simple
path b→a that uses only edges of index > s, closed by s. The outer
loop over s and the inner DFS (which respects index > s and
simplicity via a bitmask) therefore enumerate EVERY simple cycle
exactly once. (Soundness) Every closure reports its exact length
ln+1, which is compared against W. ∎

## 3. 2-degeneracy by peeling (`is_2deg`) - EXACT

**Claim.** The peeling (iterated removal of vertices of degree ≤ 2)
empties the graph iff the graph is 2-degenerate.

**Proof.** (⇐) In a 2-degenerate graph every subgraph has a vertex
of degree ≤ 2, so the peeling never gets stuck. (⇒) If the peeling
empties the graph, the reversed removal order is an elimination
order with back-degree ≤ 2. Confluence: if v is removable, it stays
removable after any other removal (degrees never grow), so the
outcome does not depend on the order. ∎

## 4. Reachability pruning (`pair_graftable`) - COMPLETE

**Claim.** The function returns 1 iff ∅ ≠ T(u,v) ⊆ P, where T is
the set of lengths of ALL simple u–v paths.

**Proof.** Every closure of length L ∉ P terminates with 0 (sound: a
negative witness has been found). It remains to show that pruning at
depth P_max loses no negative witness: a simple prefix of P_max
edges u→…→x with x ≠ v can only produce closures of length > P_max,
all ∉ P. Such closures exist iff v is reachable from x in the
residual graph G − (prefix vertices): the residual BFS decides
exactly this; if reachable → return 0, otherwise no extension of the
prefix closes and the pruning is legitimate. The enumeration
therefore stays complete with respect to the QUESTION (T ⊆ P), even
though it does not enumerate the long paths. ∎

## 5. Stack bounds

**(a) DFS over simple paths (`exact_long`, `pair_graftable`).**
Grouped-siblings lemma for the implementation "push all children,
then pop LIFO": at every instant the stack is partitioned into
groups G_1,…,G_m (bottom up), where G_l = not-yet-popped children of
the level-(l−1) node of the active branch. Induction: a pop removes
from the top group; pushing the children of the popped node creates
the new top group; every non-top group has lost ≥ 1 element (the one
whose pop created the group above). Hence |G_top| ≤ Δ, |G_l| ≤ Δ−1
for l < top, m ≤ n, whence top ≤ (n−1)(Δ−1)+Δ = n(Δ−1)+1 ≤ 900 for
n ≤ 31 < STACK_MAX = 4096. ∎

**(b) DFS over vertices (`quick_long`).** Here a vertex can be
pushed several times (once per incoming edge) before its first pop:
the grouped-siblings lemma does NOT apply. Correct bound: each edge
generates at most 2 pushes + 1 for the root ⟹ top ≤ 2E+1 ≤ 931 for
n ≤ 31 < QSTACK_MAX = 1024. ∎

Historical note: the distinction (a)/(b) emerged during review; the
original buffer of `quick_long` (64) was sufficient only for E ≤ 31,
a latent bug for reuse on dense graphs, since fixed.
