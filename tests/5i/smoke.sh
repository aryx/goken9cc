#!/bin/bash
# Exercises machines/5i (the ARM emulator/debugger) a bit beyond just
# loading a header (see tests/libmach/smoke.sh for that): symbol
# resolution + disassembly, and actually running a real Plan9 ARM
# executable to completion through the CPU emulator + syscall layer.
#
# This is a good libmach regression target too: 5i links against
# lib_toolchain/libmach, so getting a wrong disassembly/symbol/
# register-access result here would mean access.c/sym.c/machdata.c/
# 5db.c (the emulator's Machdata table) regressed.

set -e

FIVEI=${FIVEI:-5i}
EXE=${EXE:-"$(dirname "$0")/../s/mini/hello_plan9_arm.exe"}

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

# --- disassemble the real entry instruction and confirm the symbol
# table resolves it to _main (exercises sym.c + 5db.c's disassembler
# together, at a real address decoded from the real executable
# header). ---
out=$(printf '0x1020?i\n$q\n' | timeout 5 "$FIVEI" "$EXE" 2>&1)
if echo "$out" | grep -q "^_main?" && echo "$out" | grep -q "MOVW"; then
    ok "5i: disassembles entry instruction with correct symbol (_main)"
else
    bad "5i: entry disassembly/symbol resolution wrong (got: $out)"
fi

# --- actually run the program to completion via the CPU emulator +
# syscall_posix.c, and confirm it reaches exit() via the symbol
# table. Note: ':r' (reset+run) is currently broken independently of
# libmach -- it wipes the segment table via reset() without rebuilding
# it, so any fresh session must use ':c' (continue from the state
# main() already set up) instead. ---
out=$(printf ':c\n$q\n' | timeout 5 "$FIVEI" "$EXE" 2>&1)
if echo "$out" | grep -q "exits(0)" && echo "$out" | grep -q "stopped at .* exit"; then
    ok "5i: runs real Plan9/ARM executable to completion (exit syscall reached)"
else
    bad "5i: failed to run to completion (got: $out)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
