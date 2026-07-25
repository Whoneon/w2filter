#!/bin/sh
# Stage statistics over seeded random windows. For each round, draws
# (n, m, W, P) inside the supported domain, streams the FULL geng
# class through the filter and records the per-stage counters as one
# CSV row on stdout (W and P use ';' as inner separator).
# Usage: sh tests/window_stats.sh [SEED] [ROUNDS] > stats.csv
# The figure in paper/ is produced from this output by
# paper/fig_quickkill.py.
command -v nauty-geng >/dev/null 2>&1 || {
  echo "ERROR: nauty-geng not found (install nauty, e.g. 'apt install nauty')" >&2
  exit 1
}
SEED=${1:-20260725}
ROUNDS=${2:-500}
echo "round,n,m,W,P,Lmax,seen,quick,exact,deg2,survivors"
i=0
while [ "$i" -lt "$ROUNDS" ]; do
  i=$((i+1))
  set -- $(awk -v s="$SEED" -v r="$i" 'BEGIN{srand(s+r);
    n=6+int(rand()*5);                      # n in 6..10
    maxm=n*(n-1)/2; hi=(2*n+2<maxm?2*n+2:maxm);
    m=n-1+int(rand()*(hi-n+2));             # m in n-1..min(2n+2,maxm)
    w=""; L=0;
    for(l=3;l<=n;l++) if(rand()<0.5){w=w (w==""?"":";") l; L=l}
    if(w==""){L=3+int(rand()*(n-2)); w=L}
    p="";
    for(l=1;l<n;l++) if(rand()<0.5) p=p (p==""?"":";") l;
    if(p=="") p=1+int(rand()*(n-1));
    print n, m, w, p, L}')
  n=$1; m=$2; W=$3; P=$4; L=$5
  Wc=$(echo "$W" | tr ';' ',')
  Pc=$(echo "$P" | tr ';' ',')
  st=$(nauty-geng -q "$n" "$m:$m" 2>/dev/null | \
       ./w2filter "$n" --cycles "$Wc" --pairset "$Pc" 2>&1 >/dev/null | \
       awk -F'[= ]' '/^STATS:/{print $3","$5","$7","$9","$11}')
  if [ -z "$st" ]; then
    echo "window_stats: no STATS for n=$n m=$m W=$Wc P=$Pc" >&2
    exit 1
  fi
  echo "$i,$n,$m,$W,$P,$L,$st"
done
