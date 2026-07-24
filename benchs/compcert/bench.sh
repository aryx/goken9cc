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

# One column per variant: baseline shows a plain time, everything else
# shows "time (Nx)" so the speedup rides along with the number it's
# relative to instead of living in its own hard-to-scan column.
colw=18
fmt="%-12s"
header=(bench)
sepline=(------------)
for v in "${VARIANTS[@]}"; do
	fmt+=" %${colw}s"
	header+=("$v")
	sepline+=("$(printf '%*s' "$colw" '' | tr ' ' -)")
done
fmt+="\n"
printf "$fmt" "${header[@]}"
printf "$fmt" "${sepline[@]}"

# logsum[v]/nprogs accumulates ln(speedup) per non-baseline variant, so the
# final row can report a geometric mean -- the usual way to average ratios
# in a benchmark suite, since it isn't skewed by the one or two long-running
# programs the way an arithmetic mean of raw seconds would be.
declare -A logsum=()
nprogs=0

for prog in "$@"; do
	declare -A t=()
	for v in "${VARIANTS[@]}"; do
		t[$v]=$(time_one "$prog.$v")
	done
	base=${t[$BASELINE]}
	nprogs=$((nprogs + 1))
	row=("$prog")
	for v in "${VARIANTS[@]}"; do
		if [[ $v == "$BASELINE" ]]; then
			row+=("${t[$v]}s")
			continue
		fi
		speedup=$(awk -v a="$base" -v b="${t[$v]}" 'BEGIN{ printf "%.4f", (b>0 ? a/b : 0) }')
		logsum[$v]=$(awk -v s="${logsum[$v]:-0}" -v x="$speedup" 'BEGIN{ printf "%.6f", s + log(x) }')
		row+=("$(awk -v t="${t[$v]}" -v x="$speedup" 'BEGIN{ printf "%ss (%.2fx)", t, x }')")
	done
	printf "$fmt" "${row[@]}"
done

printf "$fmt" "${sepline[@]}"
avgrow=("geomean" "(baseline)")
for v in "${VARIANTS[@]:1}"; do
	avgrow+=("$(awk -v s="${logsum[$v]}" -v n="$nprogs" 'BEGIN{ printf "%.2fx", exp(s/n) }')")
done
printf "$fmt" "${avgrow[@]}"
