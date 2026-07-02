#!/bin/bash
# Test that rc's backquote command substitution `{...} runs the command
# in the forked child and captures its output into a word list, instead
# of leaking it straight to the parent's stdout.
#
# Bug: outcode()'s '`' case in rc/code.c expects c0 to be an optional
# explicit split-word list (nil defaults to $ifs) and c1 to be the
# command body run in the forked child. But rc/syn.y's grammar for the
# plain `{...} form only ever produced a one-child tree1('`', body),
# which stores the body in c0 and leaves c1 nil. outcode() then took
# the "explicit split" branch, compiling the real body as if it were
# the split-word computation: it ran directly in the current process
# (not forked), writing straight to the real stdout, while the forked
# child (running the nil c1) did nothing and produced no output. Every
# `x=`{cmd}` substitution silently became a no-op capture (0 words)
# with cmd's output leaking to the terminal instead. This broke the
# principia-softwarica 9syscall build, which relies on
# SYS=`{grep ... | sed ...} to generate syscall stubs.

set -e

RC=${RC:-rc}
PASS=0
FAIL=0

run_rc_test() {
    local name="$1"
    local script="$2"
    local expected="$3"

    local actual status
    actual=$("$RC" -c "$script" 2>/dev/null) && status=$? || status=$?

    if [ "$actual" != "$expected" ]; then
        echo "FAIL: $name (expected '$expected', got '$actual', exit $status)"
        FAIL=$((FAIL + 1))
    else
        echo "PASS: $name"
        PASS=$((PASS + 1))
    fi
}

# --- Test 1: word count of a multi-word backquote substitution ---
run_rc_test "backquote splits output into words" \
    'x=`{echo a b c}; echo $#x' "3"

# --- Test 2: captured words are usable individually ---
run_rc_test "backquote word is indexable" \
    'x=`{echo a b c}; echo $x(2)' "b"

# --- Test 3: the substituted command must NOT write to the real stdout ---
# This is the exact symptom of the regression: the command's own output
# leaked directly to the parent's stdout instead of being captured, so
# the actual captured output on stdout was polluted with the command's
# raw output before the real result.
run_rc_test "backquote output does not leak to parent stdout" \
    'x=`{echo leaked}; echo count:$#x' "count:1"

# --- Test 4: multi-line output splits on newlines (real-world pattern,
# similar to principia's `SYS=`{grep ... | sed ...}` syscall generation) ---
run_rc_test "backquote splits multi-line piped output" \
    'x=`{echo NOP; echo RFORK; echo EXEC}; echo $#x $x(1) $x(3)' "3 NOP EXEC"

# --- Test 5: command producing no output yields an empty (0-word) list ---
run_rc_test "backquote with no output gives empty list" \
    'x=`{true}; echo $#x' "0"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
