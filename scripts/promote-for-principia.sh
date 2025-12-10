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

cp ROOT/$GOARCH/bin/8c_ bin/8c
cp ROOT/$GOARCH/bin/8a_ bin/8a
cp ROOT/$GOARCH/bin/8l_ bin/8l
cp ROOT/$GOARCH/bin/ar_ bin/iar
cp ROOT/$GOARCH/bin/yacc bin/iyacc

# still? for libc/9sys/
# cp ROOT/$GOARCH/bin/sed bin/sed
