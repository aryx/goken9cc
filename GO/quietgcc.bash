#!/usr/bin/env bash
# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# The master for this file is $GOROOT/GO/quietgcc.bash
# Changes made to $GOBIN/quietgcc will be overridden.

# Gcc output that we don't care to see.
ignore=': error: .Each undeclared identifier'
ignore=$ignore'|: error: for each function it appears'
ignore=$ignore'|is dangerous, better use'
ignore=$ignore'|is almost always misused'
ignore=$ignore'|: In function '
ignore=$ignore'|: At top level: '
ignore=$ignore'|In file included from'
ignore=$ignore'|        from'

# Figure out which cc to run; this is set by make.bash.
gcc="@CC@"
if test "$gcc" = "@C""C@"; then
  gcc=gcc
fi

# Build 64-bit binaries on 64-bit systems, unless GOHOSTARCH=386.
case "$(uname -m -p)-$GOHOSTARCH" in
*x86_64*-386 | *amd64*-386)
	gcc="$gcc -m32"
	;;
*x86_64* | *amd64*)
	gcc="$gcc -m64"
#TODO? does not seem to help though, still "betypeinit error in 5g"
#*aarch64*)
#        gcc=arm-linux-gnueabihf-gcc	
esac

GCCWNO="-Wno-sign-compare -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wno-switch -Wno-comment"

case "$GOHOSTOS" in
    linux)
	#TODO: fix the warnings instead of adding more -Wno!
	GCCWNOEXTRA="-Wno-cpp -Wno-use-after-free -Wno-sizeof-array-div -Wno-unused-but-set-variable -Wno-sizeof-pointer-memaccess -Wno-sizeof-pointer-memaccess -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-aggressive-loop-optimizations -Wno-array-bounds -Wno-discarded-qualifiers -Wno-stringop-truncation"
	;;
    darwin)
	;;
    *)
	;;
esac

case "$GOHOSTOS" in
    darwin)
	# this is actually clang, not gcc
	GCCLDFLAGS=
	;;
    *)
       #TODO: ugly, should instead fix those multiple defs and fix use of EXTERN
       #update: I think it has been fixed so we can probably remove it
       GCCDFLAGS=-Wl,--allow-multiple-definition
        ;;
esac

# Run gcc, save error status, redisplay output without noise, exit with gcc status.
tmp=/tmp/qcc.$$.$USER.out
#pad: I added -DGOLANG now that the linkers use those ifdefs
$gcc -Wall -DGOLANG $GCCWNO $GCCWNOEXTRA $GCCDFLAGS "$@" >$tmp 2>&1
status=$?
grep -E -v "$ignore" $tmp | uniq | tee $tmp.1

# Make incompatible pointer type "warnings" stop the build.
# Not quite perfect--we should remove the object file--but
# a step in the right direction.
if grep -E 'incompatible pointer type' $tmp.1 >/dev/null; then
	status=1
fi
rm -f $tmp $tmp.1
exit $status
