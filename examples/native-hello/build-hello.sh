#!/bin/sh
# Build hello.c with this repo's goken9cc (see CLAUDE.md: configure, build-mk, promote, make install).
#
# Targets macOS: Mach-O x86_64 (GOOS=darwin, GOARCH=amd64) via 6c/6a/6l. Only the amd64 linker
# implements a usable Mach-O header in this tree; arm64 Mach-O is not wired in 7l.
#
# Uses the same tiny runtime as tests/c/mini (start_amd64.s, xwrite_amd64.s, xexit_amd64.s) so we
# do not need u.h (the Plan 9 preprocessor in [67]c does not accept all of include/u.h on macOS).
#
# On Apple Silicon you get an x86_64 binary (run under Rosetta if enabled). Some macOS versions
# report Rosetta errors for statically linked Plan 9-style Mach-O; an Intel Mac is the most
# reliable place to run ./hello.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
MINI=$TOP/tests/c/mini

# Mach-O output: always amd64 object pipeline (see src/cmd/6l/obj.c: darwin -> -H6).
GOOS=darwin
GOARCH=amd64
CC=6c
AS=6a
LD=6l
O=6

BIN=
for d in "$TOP/ROOT/arm64/bin" "$TOP/ROOT/amd64/bin"; do
	if test -x "$d/$CC" && test -x "$d/$LD"; then
		BIN=$d
		break
	fi
done
if test -z "$BIN"; then
	echo "missing $CC/$LD under ROOT/*/bin — build and install the toolchain first." >&2
	exit 1
fi

export GOROOT=$TOP/ROOT
export GOOS
export GOARCH
export PATH="/usr/bin:/bin:$TOP/bin:$BIN:$PATH"

START=$MINI/start_amd64.s
XW=$MINI/xwrite_amd64.s
XE=$MINI/xexit_amd64.s
for f in "$START" "$XW" "$XE"; do
	if ! test -f "$f"; then
		echo "missing $f" >&2
		exit 1
	fi
done

cd "$HERE"
rm -f hello "hello.$O" libhello.a \
	"start_$GOARCH.$O" "xwrite_$GOARCH.$O" "xexit_$GOARCH.$O"

"$BIN/$AS" -D$GOARCH -c "$START"
"$BIN/$AS" -D$GOARCH -c "$XW"
"$BIN/$AS" -D$GOARCH -c "$XE"
"$BIN/ar_" rc libhello.a "start_$GOARCH.$O" "xwrite_$GOARCH.$O" "xexit_$GOARCH.$O"
"$BIN/$CC" -D$GOARCH -I"$TOP/include" hello.c
"$BIN/$LD" -L. -o hello -E _start "hello.$O"

echo "Built $HERE/hello (GOOS=$GOOS GOARCH=$GOARCH, Mach-O x86_64)."
case $(uname -s)/$(uname -m) in
Darwin/arm64 | Darwin/aarch64)
	echo "Host is Apple Silicon: run ./hello with Rosetta if needed; if it aborts, try an Intel Mac." >&2
	;;
Darwin/*) ;;
*)
	echo "Built on $(uname -s): hello is a macOS binary; copy to a Mac to run." >&2
	;;
esac
