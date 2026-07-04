#!/bin/sh
#claude: script written by Claude Code
#
# Build the full principia world TWICE, once with each toolchain
# lineage, and compare everything that comes out byte-for-byte:
# the libraries built by iar (D modifier: deterministic member
# headers), the executables linked by 8l, and the kernel. This
# exercises the whole pipeline (8c/8a -> iar -> 8l), unlike
# cmp-c-corpus.sh / cmp-s-corpus.sh which stop at single object files.
#
#  - principia lineage: 8a/8c/8l (assemblers/8a, compilers/8c+cc,
#    linkers/8l), synced with the principia books via syncweb; the
#    default install names, used directly by the principia mkfiles
#  - kencc lineage: 8ak/8ck/8lk (assemblers/8ak, compilers/8ck+cck,
#    linkers/8lk), known good, the original reference; shimmed under
#    the plain names in a private bin dir for its build
#
# Both builds run in the same tree (mk nuke in between), so the cwd
# recorded in the object history is identical and no -r flag is needed.
# The one intentionally non-reproducible input, the kernel build date
# (kernel/COMPILE/9/pc/mkfile compiles $CONF.c with -DKERNDATE=`{date -n}),
# is pinned to a fixed value via MKFLAGS=DATE=... so the kernels compare.
#
# Usage:
#   cd <goken9cc root> && source env.sh
#   tests/scripts/cmp-exec-corpus.sh [principia-dir] [results-dir]
#
# Results:
#   386.kencc/ 386.prin/   snapshots of principia's ROOT/arch/386
#   kernel.kencc/ kernel.prin/  snapshots of the built kernels
#   build.kencc.log build.prin.log
#   match.txt differ.txt onlyone.txt   per released file

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-exec-corpus}

for t in 8a 8c 8l 8ak 8ck 8lk iar mk; do
	if ! command -v $t >/dev/null; then
		echo "error: $t must be in PATH (source env.sh first)" >&2
		exit 1
	fi
done

mkdir -p $OUT

# shim dir mapping the plain tool names to the kencc lineage
SHIM=$OUT/bin.kencc
mkdir -p $SHIM
for t in 8a 8c 8l 5a 5c 5l; do
	command -v ${t}k >/dev/null && ln -sf $(command -v ${t}k) $SHIM/$t
done

build() {
	side=$1
	log=$OUT/build.$side.log
	echo "== building principia with $side lineage (log: $log)"
	(cd $TOP &&
	 mk nuke &&
	 mk install &&
	 MKFLAGS=DATE=1000000000 mk kernel) > $log 2>&1
	if [ $? -ne 0 ]; then
		echo "error: $side build failed, see $log" >&2
		exit 1
	fi
	rm -rf $OUT/386.$side $OUT/kernel.$side
	cp -a $TOP/ROOT/arch/386 $OUT/386.$side
	mkdir -p $OUT/kernel.$side
	cp $TOP/kernel/COMPILE/9/pc/9* $OUT/kernel.$side/ 2>/dev/null
}

PATH=$SHIM:$PATH build kencc
build prin

# compare the two snapshots file by file
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onlyone.txt

for snap in 386 kernel; do
	(cd $OUT/$snap.kencc && find . -type f | sed 's,^\./,,' | sort) | while read f; do
		if [ ! -f $OUT/$snap.prin/$f ]; then
			echo "$snap/$f (kencc only)" >> $OUT/onlyone.txt
		elif cmp -s $OUT/$snap.kencc/$f $OUT/$snap.prin/$f; then
			echo "$snap/$f" >> $OUT/match.txt
		else
			echo "$snap/$f" >> $OUT/differ.txt
		fi
	done
	(cd $OUT/$snap.prin && find . -type f | sed 's,^\./,,' | sort) | while read f; do
		[ -f $OUT/$snap.kencc/$f ] || echo "$snap/$f (prin only)" >> $OUT/onlyone.txt
	done
done

echo "results in $OUT:"
echo "  match:   $(wc -l < $OUT/match.txt)"
echo "  differ:  $(wc -l < $OUT/differ.txt)"
echo "  onlyone: $(wc -l < $OUT/onlyone.txt)"
