#!/bin/sh
# Seeded randomized differential test: draws random parameters
# (n, edge count, W, P) inside the supported domain, enumerates the
# FULL class with geng (disconnected graphs included) and compares
# the C filter against the independent networkx checker, pair lists
# included. The seed is printed; reproduce any failure with:
#   SEED=<seed> make verify-random
command -v nauty-geng >/dev/null 2>&1 || {
  echo "ERROR: nauty-geng not found (install nauty, e.g. 'apt install nauty')"
  exit 1
}
SEED=${SEED:-$(date +%s)}
PY=${PY:-python3}
ROUNDS=${ROUNDS:-3}
echo "verify-random: SEED=$SEED ROUNDS=$ROUNDS"
i=0
while [ "$i" -lt "$ROUNDS" ]; do
  i=$((i+1))
  set -- $(awk -v s="$SEED" -v r="$i" 'BEGIN{srand(s+r);
    n=6+int(rand()*4);                      # n in 6..9
    maxm=n*(n-1)/2; hi=(2*n<maxm?2*n:maxm);
    m=n-1+int(rand()*(hi-n+2));             # m in n-1..min(2n,maxm)
    w=""; for(l=3;l<=n;l++) if(rand()<0.5) w=w (w==""?"":",") l;
    if(w=="") w=3+int(rand()*(n-2));
    p=""; for(l=1;l<n;l++) if(rand()<0.5) p=p (p==""?"":",") l;
    if(p=="") p=1+int(rand()*(n-1));
    print n, m, w, p}')
  n=$1; m=$2; W=$3; P=$4
  echo "  round $i: n=$n m=$m --cycles $W --pairset $P"
  a=$(nauty-geng -q "$n" "$m:$m" 2>/dev/null | \
      ./w2filter "$n" --cycles "$W" --pairset "$P" 2>/dev/null | \
      grep "^BLOCK " | \
      sed 's/BLOCK V=[0-9]* g6=//; s/ pairs=/|/' | \
      tr -d ' ' | LC_ALL=C sort)
  b=$(nauty-geng -q "$n" "$m:$m" 2>/dev/null | \
      { $PY tests/indep_check.py "$n" --cycles "$W" --pairset "$P" \
        || echo CHECKER-ERROR; } | \
      sed 's/ /|/' | tr -d ' ' | LC_ALL=C sort)
  case "$b" in *CHECKER-ERROR*)
    echo "ERROR: checker failed to run (is networkx installed?)"
    exit 1;; esac
  if [ "$a" != "$b" ]; then
    echo "FAIL round $i: n=$n m=$m W=$W P=$P (reproduce with SEED=$SEED)"
    exit 1
  fi
done
echo "OK   verify-random: $ROUNDS random windows identical to the independent checker"
