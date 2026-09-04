#!/bin/bash
# Exercises machines/ki (the SPARC emulator/debugger) a bit beyond
# just loading a header (see tests/libmach/smoke.sh for that): symbol
# resolution + disassembly, and actually running a real Plan9 SPARC
# executable to completion through the CPU emulator + syscall layer.
#
# Same shape as tests/5i/smoke.sh and tests/vi/smoke.sh -- see those
# for the fuller rationale (this is also a good libmach regression
# target: ki links against lib_toolchain/libmach, so a wrong
# disassembly/symbol/register-access result here would mean access.c/
# sym.c/machdata.c/kdb.c regressed).

set -e

KI=${KI:-ki}
EXE=${EXE:-"$(dirname "$0")/../s/mini/hello_plan9_sparc.exe"}

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

# --- disassemble the entry instruction and confirm the symbol table
# resolves it to _main (exercises sym.c + kdb.c's disassembler
# together). ---
out=$(printf '_main?i\n$q\n' | timeout 5 "$KI" "$EXE" 2>&1)
if echo "$out" | grep -q "^_main?" && echo "$out" | grep -qi "sub"; then
    ok "ki: disassembles entry instruction with correct symbol (_main)"
else
    bad "ki: entry disassembly/symbol resolution wrong (got: $out)"
fi

# --- actually run the program to completion via the CPU emulator +
# syscall.c, and confirm it reaches exits(). ---
out=$(printf ':c\n$q\n' | timeout 5 "$KI" "$EXE" 2>&1)
if echo "$out" | grep -q "Hello, world" && echo "$out" | grep -q "exits(0)"; then
    ok "ki: runs real Plan9/SPARC executable to completion (exit syscall reached)"
else
    bad "ki: failed to run to completion (got: $out)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
