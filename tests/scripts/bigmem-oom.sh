#!/bin/sh
#claude: script written by Claude Code
#
# Regression guard for the macOS "out of memory" bug (and the closely
# related infinite-recursion bug) in the 386 compiler and linker.
#
# Background
# ----------
# 8c and 8l allocate all their working memory from a bump "hunk" arena
# refilled by gethunk(). Original Plan 9 refilled it with sbrk(). On
# macOS sbrk() is deprecated and hard-capped at ~4MB, then returns -1
# forever, so any translation unit or link that needs more arena than
# that died with "<sym>: out of memory" -- e.g. principia's
# lib_graphics/libimg/torgbv.c (8c) and its 8.jpg link (8l).
#
# The fix refills the arena with the real libc malloc() instead. That is
# only possible because the compiler/linker used to redefine malloc() as
# the arena itself; with that wrapper in place gethunk()->malloc()->
# gethunk() recursed forever and crashed on the very first source file.
# So this test guards TWO regressions at once:
#   * reverting gethunk() to sbrk()      -> OOM on the big inputs below
#   * reintroducing the malloc() wrapper -> instant stack overflow
# See compilers/cc/{compat.c,utils.c} and linkers/8l/{compat.c,utils.c}.
#
# The inputs are generated large enough to blow well past the old ~4MB
# sbrk ceiling, so on a buggy toolchain 8c/8l abort; on a fixed one they
# both exit 0.
#
# Usage:
#   cd <goken9cc root> && source env.sh   # puts 8c/8a/8l in PATH
#   tests/scripts/bigmem-oom.sh

set -e

for tool in 8c 8a 8l; do
	if ! command -v $tool >/dev/null; then
		echo "error: $tool must be in PATH (source env.sh first)" >&2
		exit 2
	fi
done

work=$(mktemp -d "${TMPDIR:-/tmp}/bigmem-oom.XXXXXX")
trap 'rm -rf "$work"' EXIT
cd "$work"

fail=0

# --- compiler: a big translation unit (many functions, many locals) ---
# ~2000 functions * ~20 nodes each is comfortably over the ~4MB arena
# ceiling that sbrk imposed on macOS.
awk 'BEGIN{
	print "int sink;";
	for(i=0;i<2000;i++){
		printf "int f%d(int x){\n", i;
		printf "  int ";
		for(j=0;j<20;j++) printf "%sv%d", (j?",":""), j;
		print ";";
		for(j=0;j<20;j++) printf "  v%d = x + %d;\n", j, j;
		printf "  sink =";
		for(j=0;j<20;j++) printf "%s v%d", (j?" +":""), j;
		print ";";
		print "  return sink;";
		print "}";
	}
}' > big.c

printf '8c   big.c (%s) ... ' "$(wc -l < big.c | tr -d ' ') lines"
if 8c -FTVw -c big.c 2>bigc.err; then
	echo ok
else
	echo FAIL; sed 's/^/    /' bigc.err; fail=1
fi

# --- linker: a big self-contained 386 program (many TEXT symbols) ---
# No libc needed: a _start that exits, plus many small functions, so the
# only thing that can make this fail is the linker running out of arena.
awk 'BEGIN{
	print "TEXT _start+0(SB), $0";
	for(j=0;j<20;j++) printf "\tMOVL\t$%d, AX\n", j;
	print "\tMOVL\t$1, AX";
	print "\tXORL\tBX, BX";
	print "\tINT\t$0x80";
	for(i=0;i<2000;i++){
		printf "TEXT f%d+0(SB), $0\n", i;
		for(j=0;j<20;j++) printf "\tMOVL\t$%d, AX\n", j;
		print "\tRET";
	}
}' > big.s

printf '8a   big.s (%s) ... ' "$(wc -l < big.s | tr -d ' ') lines"
if 8a -o big_s.8 big.s 2>biga.err; then
	echo ok
else
	echo FAIL; sed 's/^/    /' biga.err; fail=1
fi

if [ -f big_s.8 ]; then
	printf '8l   big_s.8 -> big.out ... '
	if 8l -E _start -o big.out big_s.8 2>bigl.err; then
		echo ok
	else
		echo FAIL; sed 's/^/    /' bigl.err; fail=1
	fi
fi

if [ $fail -eq 0 ]; then
	echo 'PASS: 8c and 8l handle >4MB arena without running out of memory'
else
	echo 'FAIL: toolchain ran out of memory on a large input (macOS sbrk regression?)' >&2
fi
exit $fail
