"""OPTIMISED window-2 (V,E) filter. stdin = graph6 (geng -q -f -C).
Finds: blocks with cycles in {3,5,6,7} (no C4, guaranteed by -f, no
cycle >= 8), 2-degenerate; reports the graftable pairs of each.
Same-design stdlib port of the C pipeline: a fast smoke check, NOT
independent evidence (see tests/indep_check.py for that). Default
window only. Usage: geng ... | python reference_filter.py V
"""
import argparse
import sys

ap = argparse.ArgumentParser(
    description="Same-design stdlib port of the w2filter pipeline "
                "(default window only): a fast smoke check, not "
                "independent evidence. Reads graph6 from stdin.")
ap.add_argument("N", nargs="?", type=int, default=19,
                help="expected number of vertices (default 19)")
N = ap.parse_args().N

def parse_g6(line, n):
    data = [ord(c) - 63 for c in line[1:]] if ord(line[0]) - 63 == n else None
    if data is None:
        data = [ord(c) - 63 for c in line[1:]]
    bits = []
    for d in data:
        for k in range(5, -1, -1):
            bits.append((d >> k) & 1)
    adj = [[] for _ in range(n)]
    idx = 0
    for j in range(1, n):
        for i in range(j):
            if bits[idx]:
                adj[i].append(j); adj[j].append(i)
            idx += 1
    return adj

def quick_long_cycle(adj, n):
    """DFS: back-edge with depth gap >= 7 => cycle >= 8 (fast test,
    sufficient only)."""
    depth = [-1] * n
    parent = [-1] * n
    stack = [(0, -1, 0)]
    while stack:
        v, p, d = stack.pop()
        if depth[v] >= 0: continue
        depth[v] = d; parent[v] = p
        for w in adj[v]:
            if depth[w] < 0:
                stack.append((w, v, d + 1))
            elif w != p and depth[w] >= 0 and d - depth[w] >= 7:
                return True
    return False

def has_cycle_ge8_exact(adj, n):
    """canonical cycle enumeration by minimum edge, early exit at the
    first length >= 8. Implicit cap n."""
    edges = sorted({(min(u, w), max(u, w)) for u in range(n)
                    for w in adj[u]})
    eidx = {e: i for i, e in enumerate(edges)}
    for start, (a, b) in enumerate(edges):
        # paths from b to a using only edges of index > start
        st = [(b, (1 << b) | (1 << a), 1)]
        while st:
            v, mask, ln = st.pop()
            for w in adj[v]:
                e = (min(v, w), max(v, w))
                if eidx[e] <= start: continue
                if w == a:
                    if ln + 1 >= 8: return True
                    continue
                if mask >> w & 1: continue
                st.append((w, mask | 1 << w, ln + 1))
    return False

def is_2deg(adj, n):
    deg = [len(a) for a in adj]
    alive = [True] * n
    import heapq
    h = [(deg[v], v) for v in range(n)]
    heapq.heapify(h)
    removed = 0
    dd = deg[:]
    while h:
        d, v = heapq.heappop(h)
        if not alive[v] or d != dd[v]: continue
        if d > 2: return False
        alive[v] = False; removed += 1
        for w in adj[v]:
            if alive[w]:
                dd[w] -= 1
                heapq.heappush(h, (dd[w], w))
    return removed == n

def path_len_set(adj, n, u, v):
    out = set()
    st = [(u, 1 << u, 0)]
    while st:
        x, mask, d = st.pop()
        for w in adj[x]:
            if w == v: out.add(d + 1)
            elif not (mask >> w & 1): st.append((w, mask | 1 << w, d + 1))
    return out

GOOD = {1, 3, 4, 5}
nb = nseen = 0
for line in sys.stdin:
    line = line.strip()
    if not line: continue
    nseen += 1
    if nseen % 2000000 == 0:
        print(f"...visti {nseen}", flush=True)
    adj = parse_g6(line, N)
    if quick_long_cycle(adj, N): continue
    if has_cycle_ge8_exact(adj, N): continue
    if not is_2deg(adj, N): continue
    nb += 1
    pairs = []
    for u in range(N):
        for v in range(u + 1, N):
            T = path_len_set(adj, N, u, v)
            if T and T <= GOOD:
                pairs.append((u, v, sorted(T)))
    print(f"BLOCCO-W2 V={N} g6={line} graftabili={pairs}", flush=True)
print(f"WORKER DONE: visti={nseen} blocchi-w2={nb}", flush=True)
