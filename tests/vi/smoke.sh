#!/bin/bash
# Exercises machines/vi (the MIPS emulator/debugger) a bit beyond just
# loading a header (see tests/libmach/smoke.sh for that): reads real
# instruction bytes out of the real text segment of a real Plan9 MIPS
# executable, disassembles them, and runs the program to completion
# through the CPU emulator + syscall layer.
#
# This is a good libmach regression target: vi links against
# lib_toolchain/libmach, and this exercises access.c (memory read)
# together with vdb.c/vcodas.c (the mips disassembler, ported in from
# src/libmach alongside v.c -- see lib_toolchain/libmach/mkfile).

set -e

VI=${VI:-vi}
EXE=${EXE:-"$(dirname "$0")/../s/mini/hello_plan9_mips.exe"}

PASS=0
FAIL=0

ok() { echo "PASS: $1"; PASS=$((PASS + 1)); }
bad() { echo "FAIL: $1"; FAIL=$((FAIL + 1)); }

if [ ! -f "$EXE" ]; then
    bad "prerequisite $EXE missing (run tests/s/mini's mkfile first)"
    echo ""
    echo "Results: $PASS passed, $FAIL failed"
    exit 1
fi

# --- disassemble the real first instruction of the real text segment
# (crackhdr reports txtaddr=0x4020 for this binary, matching the
# header's entry -- linkers/vl rounds up to a 16K page for -H2 mips,
# see obj.c's INITRND, and lib_toolchain/libmach/v.c's mmips.pgsize
# must match that, not a generic 4K page). hello_plan9_mips.s opens
# with a stack-frame-adjusting addi, same instruction the
# assembler/linker actually emitted. ---
out=$(printf '0x4020?i\n$q\n' | timeout 5 "$VI" "$EXE" 2>&1)
if echo "$out" | grep -qi "addi.*r29"; then
    ok "vi: disassembles real first instruction of the text segment"
else
    bad "vi: text-segment disassembly wrong (got: $out)"
fi

# --- actually run the program to completion via the CPU emulator +
# syscall.c, and confirm it reaches exits(). ---
out=$(printf ':c\n$q\n' | timeout 5 "$VI" "$EXE" 2>&1)
if echo "$out" | grep -q "hello world" && echo "$out" | grep -q "exits(0)"; then
    ok "vi: runs real Plan9/MIPS executable to completion (exit syscall reached)"
else
    bad "vi: failed to run to completion (got: $out)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
