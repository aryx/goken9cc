#!/bin/sh
#claude: script written by Claude Code
#
# ARM counterpart of cmp-exec-corpus.sh: build the full principia arm
# world TWICE, once with each toolchain lineage, and compare everything
# that comes out byte-for-byte (the libraries built by iar, the command
# executables linked by 5l, and the bcm/9pi kernel). This exercises the
# whole arm pipeline (5c/5a -> iar -> 5l).
#
# Same convention as x86 (arm was swapped once it reached byte parity):
#  - principia lineage: 5a/5c/5l (assemblers/5a, compilers/5c+cc,
#    linkers/5l), synced with the principia books via syncweb; the DEFAULT
#    install names, used directly by the principia mkfiles -> built with no
#    shim.
#  - kencc lineage: 5ak/5ck/5lk (assemblers/5ak, compilers/5ck+cck,
#    linkers/5lk), known good, the original reference; shimmed under the
#    plain names in a private bin dir for its build.
#
# Both builds run in the same tree (mk nuke in between), so the cwd
# recorded in the object history is identical and no -r flag is needed.
# principia's ARFLAGS=vuD already makes iar archive member headers
# deterministic. The kernel build date (-DKERNDATE=`{date -n}) is pinned
# via MKFLAGS=DATE=... so the kernels compare.
#
# Usage:
#   cd <goken9cc root> && source env.sh
#   # principia's mkconfig must be objtype=arm
#   tests/scripts/cmp-exec-corpus-arm.sh [principia-dir] [results-dir]
#
# Results:
#   arm.kencc/ arm.prin/       snapshots of principia's ROOT/arch/arm
#   kernel.kencc/ kernel.prin/  snapshots of the built bcm kernels
#   build.kencc.log build.prin.log
#   match.txt differ.txt onlyone.txt   per released file

TOP=${1:-$HOME/github/principia-softwarica}
OUT=${2:-/tmp/cmp-exec-corpus-arm}

for t in 5a 5c 5l 5ak 5ck 5lk iar mk; do
	if ! command -v $t >/dev/null; then
		echo "error: $t must be in PATH (source env.sh first)" >&2
		exit 1
	fi
done

if ! grep -q '^objtype=arm' $TOP/mkconfig 2>/dev/null; then
	echo "error: $TOP/mkconfig must be objtype=arm" >&2
	exit 1
fi

mkdir -p $OUT

# shim dir mapping the plain tool names to the kencc lineage
SHIM=$OUT/bin.kencc
mkdir -p $SHIM
for t in 5a 5c 5l; do
	command -v ${t}k >/dev/null && ln -sf $(command -v ${t}k) $SHIM/$t
done

build() {
	side=$1
	log=$OUT/build.$side.log
	echo "== building principia arm with $side lineage (log: $log)"
	(cd $TOP &&
	 mk nuke &&
	 mk install &&
	 MKFLAGS=DATE=1000000000 mk kernel) > $log 2>&1
	rc=$?
	# install must have populated ROOT/arch/arm even if the kernel step
	# is flaky; snapshot whatever built and report the rc.
	rm -rf $OUT/arm.$side $OUT/kernel.$side
	cp -a $TOP/ROOT/arch/arm $OUT/arm.$side 2>/dev/null
	mkdir -p $OUT/kernel.$side
	cp $TOP/kernel/COMPILE/9/bcm/9* $OUT/kernel.$side/ 2>/dev/null
	if [ $rc -ne 0 ]; then
		echo "warning: $side build returned $rc, see $log (comparing what built)" >&2
	fi
}

# kencc is now shimmed under the plain names (5ak/5ck/5lk -> 5a/5c/5l)
PATH=$SHIM:$PATH build kencc
# principia is the default (plain names), used directly with no shim
build prin

# compare the two snapshots file by file
: > $OUT/match.txt
: > $OUT/differ.txt
: > $OUT/onlyone.txt

for snap in arm kernel; do
	(cd $OUT/$snap.kencc 2>/dev/null && find . -type f | sed 's,^\./,,' | sort) | while read f; do
		if [ ! -f $OUT/$snap.prin/$f ]; then
			echo "$snap/$f (kencc only)" >> $OUT/onlyone.txt
		elif cmp -s $OUT/$snap.kencc/$f $OUT/$snap.prin/$f; then
			echo "$snap/$f" >> $OUT/match.txt
		else
			echo "$snap/$f" >> $OUT/differ.txt
		fi
	done
	(cd $OUT/$snap.prin 2>/dev/null && find . -type f | sed 's,^\./,,' | sort) | while read f; do
		[ -f $OUT/$snap.kencc/$f ] || echo "$snap/$f (prin only)" >> $OUT/onlyone.txt
	done
done

echo "results in $OUT:"
echo "  match:   $(wc -l < $OUT/match.txt)"
echo "  differ:  $(wc -l < $OUT/differ.txt)"
echo "  onlyone: $(wc -l < $OUT/onlyone.txt)"
[ -s $OUT/differ.txt ] && { echo "--- differ ---"; cat $OUT/differ.txt; }
