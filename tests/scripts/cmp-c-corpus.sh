#!/bin/sh
#claude: script written by Claude Code
#
# Compile every .c file of the principia corpus with both C compiler
# lineages and compare the object files byte-for-byte:
#  - the kencc lineage (8c, from compilers/8ck), known good
#  - the principia-synced lineage (8c___, from compilers/8c), kept in
#    sync with the principia books via syncweb
#
# This is how the mismatches behind the tests in tests/c/variants were
# found (see the comment at the top of each test there).
#
# Usage:
#   cd <goken9cc root> && source env.sh   # puts 8c and 8c___ in PATH
#   tests/scripts/cmp-c-corpus.sh [principia-dir] [results-dir]
#
# Results (one file path per line):
#   match.txt     both compile, identical objects
#   differ.txt    both compile, objects differ  <- the interesting ones
#   onefail.txt   one lineage compiles, the other errors out
#   bothfail.txt  neither compiles under these x86 flags (arm-only or
#                 kernel code needing other includes; not a mismatch)

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-c-corpus}

if ! command -v 8c >/dev/null || ! command -v 8c___ >/dev/null; then
	echo 'error: 8c and 8c___ must be in PATH (source env.sh first)' >&2
	exit 1
fi

mkdir -p $OUT
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onefail.txt
: > $OUT/bothfail.txt

# the flags principia itself compiles with (mkfiles/mkfile.proto),
# plus -r (reproducible: no cwd in the object history)
CFLAGS="-FTVw -r -I$TOP/include/arch/386 -I$TOP/include/ALL"

find $TOP -name '*.c' | sort | while read f; do
	d=$(dirname "$f")
	8c    $CFLAGS -I"$d" -o $OUT/k.8 "$f" >/dev/null 2>&1; rk=$?
	8c___ $CFLAGS -I"$d" -o $OUT/p.8 "$f" >/dev/null 2>&1; rp=$?
	if [ $rk -eq 0 ] && [ $rp -eq 0 ]; then
		if cmp -s $OUT/k.8 $OUT/p.8; then
			echo "$f" >> $OUT/match.txt
		else
			echo "$f" >> $OUT/differ.txt
		fi
	elif [ $rk -ne 0 ] && [ $rp -ne 0 ]; then
		echo "$f" >> $OUT/bothfail.txt
	else
		echo "$f k=$rk p=$rp" >> $OUT/onefail.txt
	fi
done
rm -f $OUT/k.8 $OUT/p.8

echo "results in $OUT:"
echo "  match:    $(wc -l < $OUT/match.txt)"
echo "  differ:   $(wc -l < $OUT/differ.txt)"
echo "  onefail:  $(wc -l < $OUT/onefail.txt)"
echo "  bothfail: $(wc -l < $OUT/bothfail.txt)"
