#!/bin/sh
#claude: script written by Claude Code
#
# Compile every .c file of the principia corpus with both C compiler
# lineages and compare the object files byte-for-byte. This is how the
# mismatches behind the tests in tests/c/variants (and, for arm, some
# in tests/s/variants) were found.
#
# Usage:
#   cd <goken9cc root> && source env.sh
#   tests/scripts/cmp-c-corpus.sh [principia-dir] [results-dir] [arch]
#
# arch is 386 (default) or arm. Note the asymmetric naming: on x86 the
# principia-synced lineage owns the plain names (8c) and kencc is 8ck,
# while on arm (not swapped yet) kencc owns 5c and the principia one
# is 5c___.
#
# Results (one file path per line):
#   match.txt     both compile, identical objects
#   differ.txt    both compile, objects differ  <- the interesting ones
#   onefail.txt   one lineage compiles, the other errors out
#   bothfail.txt  neither compiles under these flags (other-arch or
#                 code needing other includes; not a mismatch)

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-c-corpus}
ARCH=${3:-386}

case $ARCH in
386)	KCC=8ck; PCC=8c;    O=8;;
arm)	KCC=5c;  PCC=5c___; O=5;;
*)	echo "error: unknown arch $ARCH (386 or arm)" >&2; exit 1;;
esac

if ! command -v $PCC >/dev/null || ! command -v $KCC >/dev/null; then
	echo "error: $KCC and $PCC must be in PATH (source env.sh first)" >&2
	exit 1
fi

mkdir -p $OUT
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onefail.txt
: > $OUT/bothfail.txt

# the flags principia itself compiles with (mkfiles/mkfile.proto),
# plus -r (reproducible: no cwd in the object history)
CFLAGS="-FTVw -r -I$TOP/include/arch/$ARCH -I$TOP/include/ALL"

find $TOP -name '*.c' | sort | while read f; do
	d=$(dirname "$f")
	$KCC $CFLAGS -I"$d" -o $OUT/k.$O "$f" >/dev/null 2>&1; rk=$?
	$PCC $CFLAGS -I"$d" -o $OUT/p.$O "$f" >/dev/null 2>&1; rp=$?
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
