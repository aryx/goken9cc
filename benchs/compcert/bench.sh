#!/usr/bin/env bash
# Times the gcc-O0 vs gcc-O3 binaries built by ./mkfile and prints a
# best-of-N wall-clock summary table.
#
# Invoked as: bash bench.sh <prog>...   (see the 'bench' target in mkfile)
#
# Uses bash's own `time` builtin (via TIMEFORMAT) rather than an external
# date/time command: this project installs its own 'date' and no 'time'
# at all under ROOT/$objtype/bin, and once that directory is ahead of the
# system ones on $PATH (as it is after `source env.sh`), an external-command
# approach would silently pick up the wrong tool. bash's `time` is a shell
# keyword, so it is immune to $PATH.
#
# REPS (default 1) controls how many times each binary is run, reporting
# the best (lowest) wall-clock time; bump it (REPS=3 bash bench.sh ...)
# for more stable numbers -- most of these benchmarks already run several
# seconds at -O0, so a single rep is usually enough for a quick summary.
set -euo pipefail

reps=${REPS:-1}

time_one() {
	local exe=$1 best="" t
	for ((n = 0; n < reps; n++)); do
		t=$( { TIMEFORMAT='%R'; time "./$exe" >/dev/null; } 2>&1 )
		if [[ -z "$best" ]] || awk -v a="$t" -v b="$best" 'BEGIN{exit !(a<b)}'; then
			best=$t
		fi
	done
	echo "$best"
}

printf '%-14s %10s %10s %9s\n' "bench" "O0(s)" "O3(s)" "O0/O3"
printf '%-14s %10s %10s %9s\n' "--------------" "----------" "----------" "---------"

for prog in "$@"; do
	t0=$(time_one "$prog.gcc-O0")
	t3=$(time_one "$prog.gcc-O3")
	speedup=$(awk -v a="$t0" -v b="$t3" 'BEGIN{ if (b+0>0) printf "%.2f", a/b; else print "n/a" }')
	printf '%-14s %10s %10s %9s\n' "$prog" "$t0" "$t3" "${speedup}x"
done
