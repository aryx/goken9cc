#!/usr/bin/env bash
# Cross-arch correctness matrix for goken's own toolchain: builds every
# GOKENPROGS benchmark for every cputype %.goken's switch supports, runs
# each one under the matching qemu-user emulator (scripts/qemu-runner),
# and checks its stdout against Results/<prog> byte-for-byte.
#
# Invoked as: bash test_goken_archs.sh <arch>... -- <prog>...
# (see the 'test_goken_archs' target in mkfile, which supplies
# $GOKENARCHS and $GOKENPROGS on either side of the '--')
#
# Same split of responsibilities as bench.sh/mkfile: mk still owns the
# actual Xc/Xa/Xl build (via 'mk -a -k cputype=<arch> goken' below --
# '-a' because %.goken's only rc dependency is the .c file, so once
# fib.goken exists mk would otherwise consider it up to date regardless
# of which *previous* arch produced it and skip rebuilding; '-k' so one
# prog's build failure doesn't abort the rest of that arch's progs, so
# the table below stays a complete arch x prog matrix instead of
# stopping partway through); this script only owns result bookkeeping
# and the table/log output.
#
# Each cell distinguishes *why* a prog didn't pass, not just pass/fail:
#   PASS      - built, ran, output matched Results/<prog> exactly
#   COMPFAIL  - %.goken didn't produce an executable (compile or link error)
#   RUNFAIL   - ran under qemu but exited nonzero (crash, trap, ...)
#   TIMEOUT   - ran past $TIMEOUTSECS without exiting (see below -- a real
#               one found this way: 386/8c's and arm/5c's dofmt.c hangs
#               formatting a %g of a small-magnitude double, e.g. printf's
#               own dtoa loop never terminating -- not just "qemu is slow")
#   MISMATCH  - ran and exited 0, but stdout didn't match Results/<prog>
# Build stderr for a COMPFAIL cell is kept in build-<arch>.log for
# follow-up instead of being dumped inline (22 progs x 7 archs would
# otherwise flood the table). TIMEOUTSECS is deliberately generous (not
# just "a few seconds"): these are unoptimized -O0-equivalent binaries
# run under qemu-user binary translation, not natively, and some progs
# (nbody, perlin, vmach, bisect, siphash24) already take 5-10+ real
# seconds natively -- see bench.sh's own goken column for that baseline.
set -uo pipefail

TOP=../..
TIMEOUTSECS=${TIMEOUTSECS:-60}

archs=()
while [[ $# -gt 0 && $1 != -- ]]; do
	archs+=("$1")
	shift
done
shift # drop the --
progs=("$@")

declare -A status=()

for arch in "${archs[@]}"; do
	echo "-- building goken for $arch --" >&2
	rm -f *.goken
	mk -a -k "cputype=$arch" goken >"build-$arch.log" 2>&1
	for p in "${progs[@]}"; do
		# Print before running, not after: a hung/slow prog (see TIMEOUT
		# above) would otherwise leave the terminal silent for up to
		# $TIMEOUTSECS with no indication of which prog it's stuck on --
		# this is what made a manual run look hung on "building 386" when
		# it was actually well past the build and sitting in integr.goken's
		# real ~60s dofmt.c hang.
		printf '   %-14s %-10s ... ' "$arch" "$p" >&2
		if [[ ! -x "$p.goken" ]]; then
			status["$arch,$p"]=COMPFAIL
			echo COMPFAIL >&2
			continue
		fi
		tmpout=$(mktemp)
		timeout "$TIMEOUTSECS" "$TOP/scripts/qemu-runner" "$arch" "./$p.goken" >"$tmpout" 2>/dev/null
		rc=$?
		if [[ $rc -eq 124 ]]; then
			status["$arch,$p"]=TIMEOUT
		elif [[ $rc -ne 0 ]]; then
			status["$arch,$p"]=RUNFAIL
		elif cmp -s "$tmpout" "Results/$p"; then
			status["$arch,$p"]=PASS
		else
			status["$arch,$p"]=MISMATCH
		fi
		echo "${status["$arch,$p"]}" >&2
		rm -f "$tmpout"
	done
done

# One column per arch, one row per prog -- same layout convention as
# bench.sh's variant columns.
colw=10
fmt="%-14s"
header=(prog)
sepline=(--------------)
for arch in "${archs[@]}"; do
	fmt+=" %${colw}s"
	header+=("$arch")
	sepline+=("$(printf '%*s' "$colw" '' | tr ' ' -)")
done
fmt+="\n"
printf "$fmt" "${header[@]}"
printf "$fmt" "${sepline[@]}"

declare -A passcount=()
declare -A totalcount=()
grandpass=0
grandtotal=0

for p in "${progs[@]}"; do
	row=("$p")
	for arch in "${archs[@]}"; do
		st=${status["$arch,$p"]}
		row+=("$st")
		totalcount[$arch]=$((${totalcount[$arch]:-0} + 1))
		grandtotal=$((grandtotal + 1))
		if [[ $st == PASS ]]; then
			passcount[$arch]=$((${passcount[$arch]:-0} + 1))
			grandpass=$((grandpass + 1))
		fi
	done
	printf "$fmt" "${row[@]}"
done

printf "$fmt" "${sepline[@]}"
sumrow=("pass/total")
for arch in "${archs[@]}"; do
	sumrow+=("${passcount[$arch]:-0}/${totalcount[$arch]:-0}")
done
printf "$fmt" "${sumrow[@]}"

echo
echo "TOTAL: $grandpass/$grandtotal passed (see build-<arch>.log for COMPFAIL details)"

[[ $grandpass -eq $grandtotal ]]
