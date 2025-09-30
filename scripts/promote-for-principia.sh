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

# let's focus on principia for raspberry pi for now (arm32)
cp ROOT/$GOARCH/bin/5c_ bin/5c
cp ROOT/$GOARCH/bin/5a_ bin/5a
cp ROOT/$GOARCH/bin/5l_ bin/5l
rm -f ROOT/$GOARCH/bin/5[acl]_
rm -f ROOT/$GOARCH/bin/5[acl]

cp ROOT/$GOARCH/bin/iar bin/iar
cp ROOT/$GOARCH/bin/yacc bin/iyacc

# for libc/9sys/
cp ROOT/$GOARCH/bin/sed bin/sed
