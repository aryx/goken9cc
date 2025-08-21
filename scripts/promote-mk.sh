#!/bin/sh

# We need this script because we can't rely on 'mk install'
# to update 'mk' because on Linux you'll get a "Text busy" error
# as you are overwriting the executable currently running.
# The same is true for 'rc'.

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

cp ROOT/$GOARCH/bin/mk bin/
cp ROOT/$GOARCH/bin/rc bin/
cp ROOT/$GOARCH/bin/ed bin/
