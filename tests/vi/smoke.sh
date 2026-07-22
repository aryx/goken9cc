#!/bin/bash
# Exercises machines/vi (the MIPS emulator/debugger) a bit beyond just
# loading a header (see tests/libmach/smoke.sh for that): reads real
# instruction bytes out of the real text segment of a real Plan9 MIPS
# executable and disassembles them.
#
# This is a good libmach regression target: vi links against
# lib_toolchain/libmach, and this exercises access.c (memory read)
# together with vdb.c/vcodas.c (the mips disassembler, ported in from
# src/libmach alongside v.c -- see lib_toolchain/libmach/mkfile).
#
# Note: running this binary to full completion (':c') currently faults
# with a TLB miss -- tests/s/mini/hello_plan9_mips.exe's crackhdr()
# result has txtaddr=0x20 but entry=0x4020 (unlike the arm binary,
# where txtaddr==entry), so machines/vi/vi.c's
# `s->base = fhdr.txtaddr - fhdr.hdrsz` text-segment formula doesn't
# cover the real entry point. Confirmed via a throwaway crackhdr-only
# harness that this is identical under -lmach_ (src/libmach,
# untouched by the libmach port) -- so it's a pre-existing vl/vi
# mips-specific issue, not a libmach regression, and out of scope
# here. tests/5i's smoke.sh covers actual run-to-completion instead,
# since that works for the arm binary.

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
# (crackhdr reports txtaddr=0x20 for this binary). hello_plan9_mips.s
# opens with a stack-frame-adjusting addi, same instruction the
# assembler/linker actually emitted. ---
out=$(printf '0x20?i\n$q\n' | timeout 5 "$VI" "$EXE" 2>&1)
if echo "$out" | grep -qi "addi.*r29"; then
    ok "vi: disassembles real first instruction of the text segment"
else
    bad "vi: text-segment disassembly wrong (got: $out)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
