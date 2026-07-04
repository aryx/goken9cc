#!/bin/sh

set -e
if [ ! -f configure ]; then
	echo 'this script must be run from the project root'
	exit 1
fi
if [ ! -f mkconfig ]; then
	echo 'you must run ./configure first'
	exit 1
fi
. ./mkconfig
set -x

# claude: the principia-synced lineage (assemblers/8a, compilers/8c+cc,
# linkers/8l) now installs directly under the plain 8a/8c/8l names,
# having been proven byte-identical to the kencc lineage (8ak/8ck/8lk)
# over the whole principia corpus (see tests/scripts/cmp-*-corpus.sh).
# This script just copies them (and iar/iyacc) into bin/ for a
# self-contained principia cross-toolchain; it replaces the GO-era
# 8x_ tools that were promoted before.
cp ROOT/$GOARCH/bin/8c bin/8c
cp ROOT/$GOARCH/bin/8a bin/8a
cp ROOT/$GOARCH/bin/8l bin/8l
cp ROOT/$GOARCH/bin/iar bin/iar
cp ROOT/$GOARCH/bin/iyacc bin/iyacc

# still? for libc/9sys/
# cp ROOT/$GOARCH/bin/sed bin/sed
