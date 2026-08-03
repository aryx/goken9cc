#!/bin/bash
# Test that rc actually EXPORTS its variables to child processes.
#
# It did not, until now: rc/unix.c's Updenv() was an empty function, and
# mkenv() (the routine that builds the char** block execvp(3) needs) was
# commented out alongside it. So "VAR=val cmd" -- and every plain
# assignment -- was visible only inside rc itself, and silently vanished
# at the process boundary. The Plan 9 half (rc/plan9.c) was fine
# throughout: there Updenv() writes each changed variable to /env/<name>,
# a namespace the kernel genuinely shares with children, so nothing had
# to be rebuilt or handed over.
#
# That asymmetry is the whole reason the Unix side needs a different
# shape. A Unix environment is a flat char** snapshot copied at exec
# time, not a shared directory, so the entire block has to be
# regenerated and installed into the global `environ` -- which is what
# rc/processes.c's Execute() picks up, since it calls Updenv() and then
# exec(), and BOOT/lib9's exec() is execvp().
#
# Why it mattered beyond rc itself: mk runs every recipe through
# $MKSHELL, which is this rc. So no mkfile recipe anywhere in the tree
# could set a variable for the command it was about to run. That is not
# hypothetical -- it forced tests/c/hello_libc/env.c to test getenv()
# against $PATH rather than against a variable the mkfile could set for
# the occasion, and it is documented in docs/claude_notes/
# notes_libc_selfhost.txt as a known trap.
#
# NOTE if this suite fails right after a change to rc: bin/rc is a
# *promoted copy* (scripts/promote-mk.sh), and $MKSHELL points at it,
# not at the freshly installed ROOT/arch/$objtype/bin/rc. A rebuilt rc
# does not take effect for mk -- or for this test, if it picks rc up
# from $PATH -- until promote-mk.sh has run.

set -e

RC=${RC:-rc}

# exported here rather than by the mkfile recipe so this script is
# self-contained: bash is then the "real parent" whose environment rc
# must import at startup and re-export, which is exactly what test 6
# checks. (Setting it from the recipe would also work now, but only
# BECAUSE of the fix under test -- a needlessly circular dependency.)
export GOKEN_RC_INHERITED=from-parent
PASS=0
FAIL=0

run_env_test() {
    local name="$1"
    local script="$2"
    local expected_out="$3"

    local actual
    actual=$("$RC" -c "$script" 2>/dev/null) || true

    if [ "$actual" != "$expected_out" ]; then
        echo "FAIL: $name (expected '$expected_out', got '$actual')"
        FAIL=$((FAIL + 1))
    else
        echo "PASS: $name"
        PASS=$((PASS + 1))
    fi
}

# --- Test 1: the headline case. A per-command assignment must reach the
# child, which has to be a genuinely external program -- printenv, not
# rc's own $VAR -- since the whole bug was that rc could see it and
# nobody else could. ---
run_env_test "VAR=val cmd reaches an external child" \
    'GOKEN_RC_ENV_TEST=hello /usr/bin/printenv GOKEN_RC_ENV_TEST' \
    "hello"

# --- Test 2: a standalone assignment must reach a LATER command too,
# not only the one it is attached to. This is the case rc/simple.c's
# own "necessary so changes don't go out again" Updenv() call covers,
# and it is the shape most mkfile recipes actually use. ---
run_env_test "a plain assignment reaches a later child" \
    'GOKEN_RC_ENV_TEST2=world
/usr/bin/printenv GOKEN_RC_ENV_TEST2' \
    "world"

# --- Test 3: a variable that was never set must not appear. Guards
# against a mkenv() that emitted garbage entries or ran off the end of
# the variable table -- both of which would still pass tests 1 and 2. ---
run_env_test "an unset variable does not appear in the child" \
    'if(! /usr/bin/printenv GOKEN_RC_ENV_ABSENT >/dev/null) echo absent
if not echo present' \
    "absent"

# --- Test 4: rc values are LISTS, and the environment is flat strings.
# rc joins the elements with SEP ('\001') on the way out and enval()
# splits on it on the way back in, so a list must survive a round trip
# through the Unix environment INTO A CHILD RC as a list -- not as one
# string with stray control characters. This is the check that pins the
# encoding rather than merely the plumbing. ---
run_env_test "a list variable round-trips into a child rc as a list" \
    'GOKEN_RC_LIST=(a b c) rc -c '\''echo $#GOKEN_RC_LIST $GOKEN_RC_LIST'\''' \
    "3 a b c"

# --- Test 5: and the raw bytes really are SEP-separated, so the
# encoding is pinned independently of rc being the reader. Test 4 alone
# would still pass if both sides used the same wrong separator. ---
run_env_test "the exported bytes are SEP-separated" \
    'GOKEN_RC_LIST=(a b c) /usr/bin/printenv GOKEN_RC_LIST | /usr/bin/tr ''\001'' '','' ' \
    "a,b,c"

# --- Test 6: the environment rc itself inherited must still be passed
# through. mkenv() rebuilds the block from rc's OWN variable table, so
# this only works because Vinit() imported the inherited environment
# into that table at startup -- an easy thing to break. ---
run_env_test "an inherited variable survives being re-exported" \
    '/usr/bin/printenv GOKEN_RC_INHERITED' \
    "from-parent"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
