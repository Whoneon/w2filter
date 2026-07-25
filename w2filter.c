/* ============================================================
 * w2filter.c — exhaustive filter for "window-2" blocks
 * ============================================================
 *
 * MATHEMATICAL CONTEXT (rho_2 = 11/7 campaign, Erdos–Gyarfas).
 * A "window-2 block" is a 2-connected graph whose cycles ALL have
 * length in {3,5,6,7} (no C4, no cycle >= 8) and which is
 * 2-degenerate. These are the candidate blocks of a minimal
 * counterexample ("violator") to the density bound rho_2 = 11/7
 * for 2-degenerate graphs with no cycle of length a power of 2.
 * The deficit def = 11(V-1) - 7E measures the distance from the
 * extremal ratio; the class (V,E) = (19,28) has def = 2.
 *
 * PURPOSE: test the prediction of the BUNDLE THEOREM
 * (TWO_THIRDS.md): no block with def <= 2 has a "graftable pair",
 * i.e. a pair (u,v) such that EVERY simple u-v path has length in
 * {1,3,4,5}. If one existed, grafting a 2-edge (a suppressed path
 * of weight 2) onto (u,v) would produce a graph with
 * gamma = 7E' - 11(V'-1) > 0 that stays window-2: an actual
 * violator. Expected output: empty class, or blocks that are all
 * saturated (graftable pairs = []).
 *
 * INPUT:  graph6 lines on stdin (typically: nauty-geng -q -f -C,
 *         which already guarantees 2-connectivity and no C4).
 * OUTPUT: one line per window-2 block found, with the list of its
 *         graftable pairs; progress counters.
 * USAGE:  nauty-geng -q -f -C 19 28:28 i/8 | ./w2filter 19
 * BUILD:  gcc -O3 -o w2filter w2filter.c   (make fast for -march=native)
 *
 * SOUNDNESS: every test is explained in its function comment.
 * The chain is: (1) fast SUFFICIENT test for "a cycle >= 8 exists"
 * (reject only, never accepts); (2) EXACT test for cycles >= 8
 * (complete canonical enumeration); (3) 2-degeneracy (peeling);
 * (4) for the survivors: graftable pairs by simple-path
 * enumeration. No sampling, no silent caps: the filter is
 * exhaustive on the input stream.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------------------------------
 * SAFETY BOUNDS (analysis in response to review, 2026-07-23).
 *
 * DFS STACK OCCUPANCY INVARIANT — precise proof for the ITERATIVE
 * implementation "push all children, then pop LIFO" (review of
 * 2026-07-23).
 *
 * (a) exact_long / pair_graftable (DFS over SIMPLE PATHS: each
 * frame carries its own bitmask, no re-visits within a branch).
 * GROUPED-SIBLINGS LEMMA: at every instant the stack is
 * partitioned, bottom to top, into groups G_1,...,G_m where
 * G_l = not-yet-popped children of the level-(l-1) node of the
 * active branch. Induction over the operations: a pop removes from
 * the top group; pushing the children of the popped node creates
 * the new top group; hence every NON-top group has already lost
 * >= 1 element (the one whose pop created the group above). Thus:
 *   |G_top| <= Delta,  |G_l| <= Delta-1 for l < top,  m <= n
 * (simple paths have <= n levels), whence
 *   top <= (n-1)(Delta-1) + Delta = n(Delta-1) + 1 <= n(n-2)+1.
 * With n <= NMAX = 31:  top <= 31*29+1 = 900 < STACK_MAX = 4096.
 *
 * (b) quick_long (DFS over VERTICES with mark-at-pop): here a
 * vertex can be pushed several times (once per incoming edge)
 * before its first pop, so the grouped-siblings lemma does NOT
 * give the right bound; the correct one is the push count:
 *   each edge {u,v} generates at most 2 pushes (one per direction,
 *   while the other endpoint is still unvisited) + 1 for the
 *   root:  top <= 2E + 1.
 * For arbitrary graphs with n <= 31: E <= 465 => top <= 931
 * <= 1024 (the size chosen for QSTACK_MAX). [The old 64 was
 * sufficient only for E <= 31: a latent bug for reuse on dense
 * graphs, found precisely while writing this proof.]
 *
 * NOTE for reuse with 64-bit masks (n <= 63): bound (a) becomes
 * 63*61+1 = 3844 < 4096 — a THIN margin: enlarge STACK_MAX;
 * bound (b) becomes 2*1953+1: enlarge QSTACK_MAX. The assertions
 * stay active even at -O3 (no -DNDEBUG in our builds); cost: one
 * branch per push, negligible.
 * ------------------------------------------------------------ */
#define NMAX 31
#define STACK_MAX 4096
#define PSTACK_MAX 8192
#define QSTACK_MAX 1024

static int N;                       /* number of vertices (<= 31)    */
static unsigned cycset =            /* ALLOWED cycles (bitmask)      */
    (1u<<3)|(1u<<5)|(1u<<6)|(1u<<7);           /* default: window-2  */
static int Lmax = 7;                /* max allowed cycle length      */
static unsigned goodset =           /* graftable path lengths        */
    (1u<<1)|(1u<<3)|(1u<<4)|(1u<<5);
static int Pmax = 5;                /* max length in goodset         */
static long long c_seen, c_quick, c_exact, c_deg, c_found; /* stages */
static unsigned adjmask[32];        /* adjacency bitmasks            */
static int adj[32][32], deg[32];    /* adjacency lists               */
static int elist_a[512], elist_b[512], ne;  /* edge list             */
static int eidx[32][32];            /* index of edge {i,j}           */

/* ------------------------------------------------------------
 * parse_g6: decode graph6 (nauty format). The bits of the upper
 * triangle of the adjacency matrix are packed 6 per character
 * (offset 63), column by column.
 * ------------------------------------------------------------ */
static int parse_g6(const char *s) {
    /* Review finding F2: COMPLETE line validation. A truncated
     * line used to read past the terminator (non-deterministic
     * phantom graphs from buffer residue). Now: exact length
     * 1 + ceil(n(n-1)/2 / 6) and graph6 charset [63,126]. */
    int n = s[0] - 63;
    if (n != N) return -1;
    int need = 1 + (n * (n - 1) / 2 + 5) / 6;
    int len = (int)strlen(s);
    if (len != need) return -1;
    for (int i = 1; i < len; i++)
        if (s[i] < 63 || s[i] > 126) return -1;
    memset(adjmask, 0, sizeof adjmask);
    memset(deg, 0, sizeof deg);
    ne = 0;
    const char *p = s + 1;
    int have = 0, cur = 0;
    for (int j = 1; j < n; j++)
        for (int i = 0; i < j; i++) {
            if (!have) { cur = *p++ - 63; have = 6; }
            have--;
            if ((cur >> have) & 1) {
                adjmask[i] |= 1u << j; adjmask[j] |= 1u << i;
                adj[i][deg[i]++] = j; adj[j][deg[j]++] = i;
                eidx[i][j] = eidx[j][i] = ne;
                elist_a[ne] = i; elist_b[ne] = j; ne++;
            }
        }
    return 0;
}

/* ------------------------------------------------------------
 * quick_long: FAST, SUFFICIENT-ONLY test for the existence of a
 * cycle of length >= 8. A DFS assigns depths; for every non-tree
 * edge (v,w) with depth(v) - depth(w) >= 7 there is a simple
 * cycle of length >= 8.
 *
 * SOUNDNESS (holds even if w is NOT an ancestor of v — iterative
 * DFS): let a = LCA(v,w) in the DFS tree. The cycle
 *      v ->(tree) a ->(tree) w ->(edge) v
 * is simple (the two tree branches share only a) and has length
 *      (depth v - depth a) + (depth w - depth a) + 1
 *   >= (depth v - depth w) + 1  >= 8,
 * because depth(a) <= depth(w). Hence "return 1" is always
 * justified. The test is NOT necessary (it may fail to fire even
 * when long cycles exist): survivors go to the exact test.
 * Empirically it rejects the vast majority of generated graphs.
 * ------------------------------------------------------------ */
static int quick_long(void) {
    int depth[32], top = 0;
    static int st_v[QSTACK_MAX], st_p[QSTACK_MAX], st_d[QSTACK_MAX];
    for (int i = 0; i < N; i++) depth[i] = -1;
    st_v[top] = 0; st_p[top] = -1; st_d[top] = 0; top++;
    while (top) {
        top--;
        int v = st_v[top], p = st_p[top], d = st_d[top];
        if (depth[v] >= 0) continue;
        depth[v] = d;
        for (int k = 0; k < deg[v]; k++) {
            int w = adj[v][k];
            if (depth[w] < 0) {
                assert(top < QSTACK_MAX);  /* bound (b): <= 2E+1 */
                st_v[top] = w; st_p[top] = v; st_d[top] = d + 1; top++;
            } else if (w != p && d - depth[w] >= Lmax) return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------
 * exact_long: EXACT test for the existence of a cycle >= 8, by
 * COMPLETE canonical enumeration of simple cycles.
 *
 * CANONICITY (exhaustiveness without repetition): every simple
 * cycle C has a unique minimum-index edge s = {a,b}; C is
 * enumerated exactly once as a simple path b -> a that uses only
 * edges of index > s, closed by edge s. The outer loop over s
 * therefore covers ALL simple cycles of the graph.
 * Early exit: at the first closure of length ln+1 outside W.
 * If the function returns 0, EVERY simple-cycle length has been
 * compared against the allowed set W (cycset): the verdict does
 * NOT depend on generator-side pre-filters (geng -f is only an
 * upstream optimisation for the C4-free default).
 * Cost: exponential in principle, but the graphs that reach this
 * stage are sparse (E = V+9) and almost all die immediately; the
 * "all cycles short" case has a small path space.
 * ------------------------------------------------------------ */
static int exact_long(void) {
    static int sv[STACK_MAX], sl[STACK_MAX];
    static unsigned sm[STACK_MAX];
    for (int s = 0; s < ne; s++) {
        int a = elist_a[s], b = elist_b[s];
        int top = 0;
        sv[top] = b; sm[top] = (1u << a) | (1u << b); sl[top] = 1; top++;
        while (top) {
            top--;
            int v = sv[top], ln = sl[top];
            unsigned mask = sm[top];
            for (int k = 0; k < deg[v]; k++) {
                int w = adj[v][k];
                if (eidx[v][w] <= s) continue;      /* canonicity  */
                if (w == a) {
                    int L = ln + 1;
                    if (L > Lmax || !(cycset >> L & 1)) return 1;
                    continue;
                }
                if (mask >> w & 1) continue;        /* simplicity  */
                assert(top < STACK_MAX);   /* invariant: top <= 900 */
                sv[top] = w; sm[top] = mask | (1u << w);
                sl[top] = ln + 1; top++;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------
 * is_2deg: 2-degeneracy by peeling. A graph is 2-degenerate iff
 * iterated removal of vertices of degree <= 2 empties it. The
 * peeling is confluent (order does not matter): a removable
 * vertex stays removable after other removals. Hence the test
 * "remove while possible, check whether anything remains" is
 * exact. (2-degeneracy is the hypothesis of the rho_2 density
 * class.)
 * ------------------------------------------------------------ */
static int is_2deg(void) {
    int dd[32], alive = N;
    unsigned am[32];
    memcpy(am, adjmask, sizeof am);
    for (int i = 0; i < N; i++) dd[i] = deg[i];
    int removed = 1;
    while (alive && removed) {
        removed = 0;
        for (int v = 0; v < N; v++) {
            if (dd[v] >= 0 && dd[v] <= 2) {
                for (int w = 0; w < N; w++)
                    if (am[v] >> w & 1) { am[w] &= ~(1u << v); dd[w]--; }
                dd[v] = -1; am[v] = 0; alive--; removed = 1;
            }
        }
    }
    return alive == 0;
}

/* ------------------------------------------------------------
 * pair_graftable: the pair (u,v) is GRAFTABLE iff the set T(u,v)
 * of lengths of ALL simple u-v paths satisfies ∅ ≠ T ⊆ {1,3,4,5}.
 * Motivation (graft/tower theory): adding a u-x-v path of weight
 * 2 creates, with every u-v path of length l, a cycle of length
 * 2+l; the window {3,5,6,7} forces l ∈ {1,3,4,5}. A def-2 block
 * with a graftable pair would give, after the graft, gamma > 0: a
 * violator of rho_2 = 11/7.
 *
 * EARLY EXIT (optimisation requested in review, semantically
 * neutral): as soon as a path of length 2 or >= 6 is found the
 * pair is disqualified and we return at once, without completing
 * the enumeration. NOTE: this function runs ONLY on survivors
 * (complete window-2 blocks), so it is off the hot path; the
 * early exit is for robustness, not throughput.
 * Returns: 1 if graftable (and fills tmask), 0 otherwise.
 * ------------------------------------------------------------ */
static int pair_graftable(int u, int v, unsigned *tmask) {
    static int sv[PSTACK_MAX], sl[PSTACK_MAX];
    static unsigned sm[PSTACK_MAX];
    unsigned out = 0;
    int top = 0;
    sv[top] = u; sm[top] = 1u << u; sl[top] = 0; top++;
    while (top) {
        top--;
        int x = sv[top], d = sl[top];
        unsigned mask = sm[top];
        for (int k = 0; k < deg[x]; k++) {
            int w = adj[x][k];
            if (w == v) {
                int L = d + 1;
                if (L > Pmax || !(goodset >> L & 1))
                    return 0;                     /* early exit: bad */
                out |= 1u << L;
                continue;
            }
            if (mask >> w & 1) continue;
            if (d + 1 == Pmax) {
                /* Simple prefix of Pmax edges u->...->w (w != v).
                 * EVERY further closure would have length > Pmax,
                 * i.e. disqualifying. Hence: if v is REACHABLE
                 * from w in the residual graph G - (prefix
                 * vertices), a simple u-v path of length > Pmax
                 * exists => the pair is NOT graftable (return 0);
                 * otherwise no extension of this prefix closes,
                 * and pruning is legitimate. The enumeration thus
                 * stays COMPLETE with respect to the question
                 * T ⊆ P without exploring deeper paths. */
                unsigned vis = mask | (1u << w), frontier = 1u << w;
                while (frontier) {
                    unsigned nxt = 0;
                    for (int q = 0; q < N; q++)
                        if (frontier >> q & 1) {
                            if (adjmask[q] >> v & 1) return 0; /* bad */
                            nxt |= adjmask[q] & ~vis;
                        }
                    vis |= nxt; frontier = nxt;
                }
                continue;
            }
            assert(top < PSTACK_MAX);  /* invariant: top <= 900 */
            sv[top] = w; sm[top] = mask | (1u << w); sl[top] = d + 1; top++;
        }
    }
    *tmask = out;
    return out != 0;
}

static unsigned parse_set(const char *arg, int *mx) {
    /* Returns 0 (empty set = error) on ANY input not of the form
     * "n1,n2,...": review finding F1 — strtol on a non-numeric
     * character returns 0 without advancing the pointer, so
     * progress must be checked explicitly. */
    unsigned m = 0; *mx = 0;
    while (*arg) {
        char *end;
        long x = strtol(arg, &end, 10);
        if (end == arg) return 0;              /* no progress       */
        if (x < 1 || x > 31) return 0;         /* out of domain     */
        m |= 1u << x; if (x > *mx) *mx = (int)x;
        arg = end;
        if (*arg == ',') { arg++; if (!*arg) return 0; }
        else if (*arg) return 0;               /* invalid separator */
    }
    return m;
}

#define W2FILTER_VERSION "1.0.0"

static void usage(FILE *f) {
    fprintf(f,
        "usage: w2filter N [--cycles a,b,...] [--pairset a,b,...]\n"
        "  N          expected number of vertices (3..31)\n"
        "  --cycles   allowed cycle lengths W (default 3,5,6,7)\n"
        "  --pairset  allowed path lengths P (default 1,3,4,5)\n"
        "reads graph6 from stdin; exit codes: 0 ok, 2 invalid\n"
        "options, 3 at least one input line discarded.\n");
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--version")) {
            printf("w2filter %s\n", W2FILTER_VERSION);
            return 0;
        }
        if (!strcmp(argv[i], "--help")) { usage(stdout); return 0; }
    }
    N = argc > 1 ? atoi(argv[1]) : 19;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--cycles") && i + 1 < argc) {
            cycset = parse_set(argv[++i], &Lmax);
            if (!cycset) { fprintf(stderr,
                "w2filter: invalid --cycles: %s\n", argv[i]); return 2; }
        } else if (!strcmp(argv[i], "--pairset") && i + 1 < argc) {
            goodset = parse_set(argv[++i], &Pmax);
            if (!goodset) { fprintf(stderr,
                "w2filter: invalid --pairset: %s\n", argv[i]); return 2; }
        } else {                                /* F6: never silent */
            fprintf(stderr, "w2filter: unknown option: %s\n",
                    argv[i]);
            usage(stderr);
            return 2;
        }
    }
    if (N < 3 || N > NMAX) {
        /* n > 31 would make the shifts on 32-bit masks UB: this
         * was the most serious missing guardrail in the file. */
        fprintf(stderr, "w2filter: N must be in [3,%d]\n", NMAX);
        return 2;
    }
    char line[256];
    long long seen = 0, found = 0, rejected = 0;
    /* Output labels (BLOCCO-W2, graftabili, visti, scartati) are
     * FROZEN: they must match the golden anchor tests and the
     * shipped production logs of the (19,28) campaign in results/.
     * See README for a legend. */
    while (fgets(line, sizeof line, stdin)) {
        int L = strlen(line);
        while (L && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = 0;
        if (!L) continue;
        seen++; c_seen++;
        if (seen % 100000000LL == 0)
            { printf("...visti %lld\n", seen); fflush(stdout); }
        if (parse_g6(line)) {
            rejected++;              /* F3: never discard silently */
            if (rejected <= 10)
                fprintf(stderr, "w2filter: invalid line %lld: "
                        "%.40s\n", seen, line);
            continue;
        }
        if (quick_long()) { c_quick++; continue; }  /* fast kill     */
        if (exact_long()) { c_exact++; continue; }  /* exact verdict */
        if (!is_2deg())   { c_deg++;   continue; }  /* class hypothesis */
        found++; c_found++;
        printf("BLOCCO-W2 V=%d g6=%s graftabili=[", N, line);
        int first = 1;
        for (int u = 0; u < N; u++)
            for (int v = u + 1; v < N; v++) {
                unsigned T;
                if (pair_graftable(u, v, &T)) {
                    printf("%s(%d,%d)", first ? "" : ", ", u, v);
                    first = 0;
                }
            }
        printf("]\n"); fflush(stdout);
    }
    printf("WORKER DONE: visti=%lld blocchi-w2=%lld scartati=%lld\n",
           seen, found, rejected);
    fprintf(stderr, "STATS: visti=%lld quick=%lld exact=%lld "
            "2deg=%lld superstiti=%lld scartati=%lld\n",
            c_seen, c_quick, c_exact, c_deg, c_found, rejected);
    /* exit 3 if lines were discarded: stream exhaustiveness is
     * part of the contract; the caller MUST notice. */
    return rejected ? 3 : 0;
}
