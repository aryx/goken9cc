#!/usr/bin/env bash
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

set -e
if [ ! -f env.bash ]; then
	echo 'make.bash must be run from $GOROOT/GO' 1>&2
	exit 1
fi
. ./env.bash

# Create target directories
if [ "$GOBIN" = "$GOROOT/bin" ]; then
	mkdir -p "$GOROOT/bin"
fi
mkdir -p "$GOROOT/pkg"
mkdir -p "$GOROOT/lib"

GOROOT_FINAL=${GOROOT_FINAL:-$GOROOT}

MAKEFLAGS=${MAKEFLAGS:-"-j4"}
export MAKEFLAGS
unset CDPATH	# in case user has it set

rm -f "$GOBIN"/quietgcc
CC=${CC:-gcc}
export CC
sed -e "s|@CC@|$CC|" < "$GOROOT"/GO/quietgcc.bash > "$GOBIN"/quietgcc
chmod +x "$GOBIN"/quietgcc

rm -f "$GOBIN"/gomake
(
	echo '#!/bin/sh'
	echo 'export GOROOT=${GOROOT:-'$GOROOT_FINAL'}'
	echo 'exec '$MAKE' "$@"'
) >"$GOBIN"/gomake
chmod +x "$GOBIN"/gomake

if [ -d /selinux -a -f /selinux/booleans/allow_execstack ] ; then
	if ! cat /selinux/booleans/allow_execstack | grep -c '^1 1$' >> /dev/null ; then
		echo "WARNING: the default SELinux policy on, at least, Fedora 12 breaks "
		echo "Go. You can enable the features that Go needs via the following "
		echo "command (as root):"
		echo "  # setsebool -P allow_execstack 1"
		echo
		echo "Note that this affects your system globally! "
		echo
		echo "The build will continue in five seconds in case we "
		echo "misdiagnosed the issue..."

		sleep 5
	fi
fi

(
	cd "$GOROOT"/GO/pkg;
	bash deps.bash	# do this here so clean.bash will work in the pkg directory
)
bash "$GOROOT"/GO/clean.bash

# pkg builds the Go programs in cmd.
for i in lib9 libbio libmach cmd pkg
do
		# The ( ) here are to preserve the current directory
		# for the next round despite the cd $i below.
		# set -e does not apply to ( ) so we must explicitly
		# test the exit status.
		(
			echo; echo; echo %%%% making $i %%%%; echo
			cd "$GOROOT"/GO/$i
			case $i in
			cmd)
				bash make.bash
				;;
			pkg)
				gomake install
				;;
			*)
				gomake install
			esac
		)  || exit 1
done

# Print post-install messages.
# Implemented as a function so that all.bash can repeat the output
# after run.bash finishes running all the tests.
installed() {
	eval $(gomake -f Make.inc go-env)
	echo
	echo ---
	echo Installed Go for $GOOS/$GOARCH in "$GOROOT".
	echo Installed commands in "$GOBIN".
	case "$OLDPATH" in
	*":$GOBIN" | *":$GOBIN:"*)
		;;
	*)
		echo '***' "You need to add $GOBIN to your "'$PATH.' '***'
	esac
	echo The compiler is $GC.
	if [ "$(uname)" = "Darwin" ]; then
		echo
		echo On OS X the debuggers must be installed setgrp procmod.
		echo Read and run ./sudo.bash to install the debuggers.
	fi
	if [ "$GOROOT_FINAL" != "$GOROOT" ]; then
		echo
		echo The binaries expect "$GOROOT" to be copied or moved to "$GOROOT_FINAL".
	fi
}

(installed)  # run in sub-shell to avoid polluting environment

