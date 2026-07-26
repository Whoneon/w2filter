# w2filter — certified exhaustive filter (rho2 = 11/7 campaign)
# See w2filter.c for mathematical context and proofs.
CC = gcc
CFLAGS = -O3 -Wall -Wextra

w2filter: w2filter.c
	$(CC) $(CFLAGS) -o $@ $<

# machine-tuned build (not portable; used for the benchmark numbers
# in the note)
fast:
	$(MAKE) CFLAGS="$(CFLAGS) -march=native" -B w2filter

# Anchors with automatic assertions: FAILS on any deviation
# (counts AND graftable-pair lists). Requires nauty-geng.
test: w2filter
	@sh tests/run_anchors.sh

# Cross-check against the INDEPENDENT checker (networkx: parser,
# simple_cycles, core_number, all_simple_paths, no algorithm in
# common). Compares NORMALISED outputs: block g6 AND pair lists.
# Dev dependency: pip install networkx
PY ?= python3
GENG_CHECK = command -v nauty-geng >/dev/null 2>&1 || \
  { echo "ERROR: nauty-geng not found (install nauty, e.g. 'apt install nauty')"; exit 1; }
verify: w2filter
	@$(GENG_CHECK); for VE in "11 15" "12 17" "14 20"; do set -- $$VE; \
	  a=$$(nauty-geng -q -f -C $$1 $$2:$$2 | ./w2filter $$1 2>/dev/null \
	      | grep "^BLOCK " \
	      | sed 's/BLOCK V=[0-9]* g6=//; s/ pairs=/|/' \
	      | tr -d ' ' | LC_ALL=C sort); \
	  b=$$(nauty-geng -q -f -C $$1 $$2:$$2 | \
	      { $(PY) tests/indep_check.py $$1 || echo CHECKER-ERROR; } \
	      | sed 's/ /|/' | tr -d ' ' | LC_ALL=C sort); \
	  case "$$b" in *CHECKER-ERROR*) \
	    echo "ERROR: checker failed to run (is networkx installed?)"; \
	    exit 1;; esac; \
	  if [ "$$a" = "$$b" ]; then echo "OK   ($$1,$$2) C == indep"; \
	  else echo "FAIL ($$1,$$2)"; exit 1; fi; done
	@echo "verify: blocks and pair lists identical to the independent checker"

# parametric verification (non-default window), same logic
verify-param: w2filter
	@$(GENG_CHECK); a=$$(nauty-geng -q -c 10 13:13 | \
	     ./w2filter 10 --cycles 3,4,5,6,7,9 --pairset 1,3,5 2>/dev/null \
	     | grep "^BLOCK " \
	     | sed 's/BLOCK V=[0-9]* g6=//; s/ pairs=/|/' \
	     | tr -d ' ' | LC_ALL=C sort); \
	 b=$$(nauty-geng -q -c 10 13:13 | \
	     { $(PY) tests/indep_check.py 10 --cycles 3,4,5,6,7,9 \
	       --pairset 1,3,5 || echo CHECKER-ERROR; } \
	     | sed 's/ /|/' | tr -d ' ' | LC_ALL=C sort); \
	 case "$$b" in *CHECKER-ERROR*) \
	   echo "ERROR: checker failed to run (is networkx installed?)"; \
	   exit 1;; esac; \
	 if [ "$$a" = "$$b" ]; then echo "OK   parametric C == indep"; \
	 else echo "FAIL parametric"; exit 1; fi

# randomized differential verification: random (n, m, W, P) drawn
# from a printed seed, full class enumerated, pair lists compared.
# Reproduce a failure with: SEED=<seed> make verify-random
verify-random: w2filter
	@SEED=$(SEED) ROUNDS=$(ROUNDS) PY=$(PY) sh tests/verify_random.sh

clean:
	rm -f w2filter w2filter_san

.PHONY: fast test verify verify-param verify-random clean
