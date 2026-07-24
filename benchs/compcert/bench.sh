#!/usr/bin/env bash
# Times every compiler/opt-level binary built by ./mkfile and prints a
# best-of-N wall-clock summary, each variant's speedup expressed relative
# to the gcc-O0 baseline (BASELINE below).
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

# Every "$stem.$variant" binary mkfile can produce, in display order.
# gcc-O0 (VARIANTS[0]) is the fixed baseline all speedup columns are
# computed against -- add new compilers/opt-levels (clang, later goken's
# %.goken-5/6/7/8) here, not by changing what the baseline is.
VARIANTS=(gcc-O0 gcc-O3 clang-O0 clang-O3)
BASELINE=${VARIANTS[0]}

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

fmt="%-14s"
header=(bench)
sepline=(--------------)
for v in "${VARIANTS[@]}"; do
	fmt+=" %10s %9s"
	header+=("$v(s)" "x")
	sepline+=(---------- ---------)
done
fmt+="\n"
printf "$fmt" "${header[@]}"
printf "$fmt" "${sepline[@]}"

for prog in "$@"; do
	declare -A t=()
	for v in "${VARIANTS[@]}"; do
		t[$v]=$(time_one "$prog.$v")
	done
	base=${t[$BASELINE]}
	row=("$prog")
	for v in "${VARIANTS[@]}"; do
		speedup=$(awk -v a="$base" -v b="${t[$v]}" 'BEGIN{ if (b+0>0) printf "%.2f", a/b; else print "n/a" }')
		row+=("${t[$v]}" "${speedup}x")
	done
	printf "$fmt" "${row[@]}"
done
