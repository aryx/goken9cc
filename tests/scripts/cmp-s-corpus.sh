#!/bin/sh
#claude: script written by Claude Code
#
# Same as cmp-c-corpus.sh but for the assemblers: assemble every .s
# file of the principia corpus with both lineages and compare the
# object files byte-for-byte. This is how the assembler mismatches
# behind tests/s/variants were found.
#
# Usage:
#   cd <goken9cc root> && source env.sh
#   tests/scripts/cmp-s-corpus.sh [principia-dir] [results-dir] [arch]
#
# arch is 386 (default) or arm. Note the asymmetric naming: on x86 the
# principia-synced lineage owns the plain names (8a) and kencc is 8ak,
# while on arm (not swapped yet) kencc owns 5a and the principia one
# is 5a___.

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-s-corpus}
ARCH=${3:-386}

case $ARCH in
386)	KAS=8ak; PAS=8a;    O=8;;
arm)	KAS=5a;  PAS=5a___; O=5;;
*)	echo "error: unknown arch $ARCH (386 or arm)" >&2; exit 1;;
esac

if ! command -v $PAS >/dev/null || ! command -v $KAS >/dev/null; then
	echo "error: $KAS and $PAS must be in PATH (source env.sh first)" >&2
	exit 1
fi

mkdir -p $OUT
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onefail.txt
: > $OUT/bothfail.txt

# -r: reproducible (no cwd in the object history)
AFLAGS="-r -I$TOP/include/arch/$ARCH -I$TOP/include/ALL"

find $TOP -name '*.s' | sort | while read f; do
	d=$(dirname "$f")
	$KAS $AFLAGS -I"$d" -o $OUT/k.$O "$f" >/dev/null 2>&1; rk=$?
	$PAS $AFLAGS -I"$d" -o $OUT/p.$O "$f" >/dev/null 2>&1; rp=$?
	if [ $rk -eq 0 ] && [ $rp -eq 0 ]; then
		if cmp -s $OUT/k.$O $OUT/p.$O; then
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
rm -f $OUT/k.$O $OUT/p.$O

echo "results in $OUT ($ARCH):"
echo "  match:    $(wc -l < $OUT/match.txt)"
echo "  differ:   $(wc -l < $OUT/differ.txt)"
echo "  onefail:  $(wc -l < $OUT/onefail.txt)"
echo "  bothfail: $(wc -l < $OUT/bothfail.txt)"
