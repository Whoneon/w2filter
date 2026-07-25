#!/bin/sh
# Anchor classes with AUTOMATIC assertions (F4: the test MUST be able
# to fail). Expected values from the historical censuses of the rho2
# campaign, since reproduced by three independent implementations.
# Falls back to the shipped datasets in tests/data/ when nauty-geng
# is not installed; the (14,20) class is too large to ship and is
# skipped in that case.
set -e
if ! ./w2filter --version >/dev/null 2>&1; then
  echo "ERROR: ./w2filter is missing or does not run on this machine"
  echo "       (stale binary from an archive?). Run 'make clean && make' first."
  exit 1
fi
fail=0
have_geng=1
command -v nauty-geng >/dev/null 2>&1 || have_geng=0
if [ "$have_geng" = 0 ]; then
  echo "WARNING: nauty-geng not found (install nauty, e.g. 'apt install nauty')."
  echo "         Using the shipped datasets; the (14,20) anchor is SKIPPED."
fi
gen() {  # gen V E -> graph6 stream on stdout
  if [ "$have_geng" = 1 ]; then
    nauty-geng -q -f -C "$1" "$2:$2"
  else
    cat "tests/data/c$1_$2.g6"
  fi
}
chk() {  # chk V E expected_blocks expected_graftable
  if [ "$have_geng" = 0 ] && [ ! -f "tests/data/c$1_$2.g6" ]; then
    echo "SKIP ($1,$2): nauty-geng not installed and no shipped dataset"
    return 0
  fi
  out=$(gen "$1" "$2" | ./w2filter "$1" 2>/dev/null)
  nb=$(echo "$out" | grep -c "^BLOCCO-W2" || true)
  ng=$(echo "$out" | grep -c "graftabili=\[(" || true)
  if [ "$nb" != "$3" ] || [ "$ng" != "$4" ]; then
    echo "FAIL ($1,$2): blocks $nb/$3 graftable $ng/$4"; fail=1
  else
    echo "OK   ($1,$2): blocks $nb, graftable $ng"
  fi
}
chk 11 15 8 3
chk 12 17 2 0
chk 14 20 2 0
# exact pair lists of the (11,15) anchor (F5: not only counts).
# LC_ALL=C pins byte collation: with a UTF-8 locale, sort may order
# these lines differently and the comparison would break.
pairs=$(gen 11 15 | ./w2filter 11 2>/dev/null | \
        grep "graftabili=\[(" | sed 's/.*graftabili=//' | \
        LC_ALL=C sort | tr -d ' ')
exp='[(3,10)]
[(4,9),(4,10)]
[(4,9)]'
if [ "$pairs" != "$exp" ]; then
  echo "FAIL pairs (11,15):"; echo "$pairs"; fail=1
else
  echo "OK   pairs (11,15) exact"
fi
exit $fail
