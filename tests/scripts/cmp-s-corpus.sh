#!/bin/sh
#claude: script written by Claude Code
#
# Same as cmp-c-corpus.sh but for the assemblers: assemble every .s
# file of the principia corpus with both lineages (8a from
# assemblers/8ak vs 8a___ from assemblers/8a) and compare the object
# files byte-for-byte. This is how the assembler mismatches behind
# tests/s/variants were found.
#
# Usage:
#   cd <goken9cc root> && source env.sh   # puts 8a and 8a___ in PATH
#   tests/scripts/cmp-s-corpus.sh [principia-dir] [results-dir]

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-s-corpus}

if ! command -v 8a >/dev/null || ! command -v 8a___ >/dev/null; then
	echo 'error: 8a and 8a___ must be in PATH (source env.sh first)' >&2
	exit 1
fi

mkdir -p $OUT
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onefail.txt
: > $OUT/bothfail.txt

# -r: reproducible (no cwd in the object history)
AFLAGS="-r -I$TOP/include/arch/386 -I$TOP/include/ALL"

find $TOP -name '*.s' | sort | while read f; do
	d=$(dirname "$f")
	8a    $AFLAGS -I"$d" -o $OUT/k.8 "$f" >/dev/null 2>&1; rk=$?
	8a___ $AFLAGS -I"$d" -o $OUT/p.8 "$f" >/dev/null 2>&1; rp=$?
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
