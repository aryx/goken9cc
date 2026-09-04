#!/bin/bash
# Test that lex doesn't segfault on a rule restricted to a %Start condition.
#
# Bug: generators/lex declared its parse tree's left[]/right[] arrays as
# int (sub2.c/globals.c/ldefs.h). For a <cond>pattern rule, the grammar
# action (parser.y's "SCON r" rule) builds an RSCON node whose right[]
# slot holds a real pointer, already cast to the pointer-wide `uintptr`
# at the call site -- but storing it into an int[] on a 64-bit host
# truncates it to 32 bits. first() (sub2.c) later reinterprets right[v]
# as that pointer and dereferences it, segfaulting on the garbage
# truncated address.
#
# Any lexer with a %Start condition and at least one <cond>pattern rule
# hits this -- e.g. principia-softwarica's typesetting/pic/picl.lx and
# typesetting/grap/grapl.lx, which is how this was first found.
#
# Fixed by widening left[]/right[] to uintptr in globals.c/ldefs.h.

set -e

LEX=${LEX:-lex}
PASS=0
FAIL=0

TESTBASE=$(mktemp -d)
trap 'rm -rf "$TESTBASE"' EXIT

cat > "$TESTBASE/start_condition.l" <<'EOF'
%Start A
%%
<A>a	;
%%
EOF

set +e
"$LEX" -t "$TESTBASE/start_condition.l" > "$TESTBASE/out.c" 2> "$TESTBASE/err.txt"
status=$?
set -e

if [ "$status" -eq 139 ] || [ "$status" -eq 134 ]; then
    echo "FAIL: lex crashed on a %Start condition rule (exit $status, likely SIGSEGV/SIGABRT)"
    cat "$TESTBASE/err.txt"
    FAIL=$((FAIL + 1))
else
    echo "PASS: lex did not crash on a %Start condition rule (exit $status)"
    PASS=$((PASS + 1))
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
