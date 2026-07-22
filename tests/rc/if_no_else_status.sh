#!/bin/bash
# Test rc's exit status when an `if(cond) cmd` has a false cond and no
# matching `if not` branch: $status is left as the *condition's own*
# (nonzero) exit status, not reset to 0, even though skipping cmd is
# perfectly correct behavior.
#
# This is not a bug in this rc port -- it is standard Plan 9 rc
# semantics (an `if` with an untaken branch simply doesn't touch
# $status, so whatever set it last -- here, the failed `~` match --
# is what remains). But it is an easy trap for mkfile authors: if such
# an `if` is the LAST line of an mk recipe, mk uses that leftover
# nonzero $status as the recipe's own exit status and marks the whole
# target as failed, even though nothing actually went wrong.
#
# This exact trap broke tests/s/mini/mkfile's hello_macos_arm64.exe
# rule:
#
#   7l -H6 -o $target -E _start hello_macos_arm64.7
#   if(~ `{uname} Darwin) codesign -s - -f $target
#
# On Linux, `uname` != Darwin, so codesign is (correctly) skipped --
# but since that `if` was the recipe's last line, mk saw the nonzero
# $status from the failed `~` comparison and treated the target as
# broken. It passed under a locally cached Docker image (the recipe
# wasn't actually re-run) but failed on a genuinely fresh build, which
# is how it was caught in CI. The fix is to always give the `if` a
# terminal `if not` branch so the recipe's last command explicitly
# succeeds regardless of which branch is taken:
#
#   if(~ `{uname} Darwin) codesign -s - -f $target
#   if not true
#
# The tests below pin down the underlying rc behavior so this gotcha
# doesn't sneak back in unnoticed and to document it for anyone
# writing new mkfile recipes that branch on a condition as their last
# step.

set -e

RC=${RC:-rc}
PASS=0
FAIL=0

# Runs $script under rc and checks both its stdout and its final
# $status, unlike backquote.sh's helper which only checks output --
# here the exit status IS the behavior under test.
run_status_test() {
    local name="$1"
    local script="$2"
    local expected_out="$3"
    local expected_status="$4"

    local actual actual_status
    actual=$("$RC" -c "$script" 2>/dev/null) && actual_status=$? || actual_status=$?

    if [ "$actual" != "$expected_out" ] || [ "$actual_status" != "$expected_status" ]; then
        echo "FAIL: $name (expected out='$expected_out' status=$expected_status, got out='$actual' status=$actual_status)"
        FAIL=$((FAIL + 1))
    else
        echo "PASS: $name"
        PASS=$((PASS + 1))
    fi
}

# --- Test 1: true condition, cmd runs and its own status is reported ---
run_status_test "if(true cond) succeeding cmd -> status 0" \
    'if(~ a a) echo yes' "yes" "0"

# --- Test 2: the gotcha itself -- false condition, no `if not`: the
# untaken `if` leaves $status as the condition test's own nonzero
# status, NOT 0, even though not running cmd is correct. ---
run_status_test "if(false cond) no else -> leftover nonzero status" \
    'if(~ a b) echo yes' "" "1"

# --- Test 3: same false condition, but with `echo $status` as a
# separate trailing statement -- confirms $status truly carries the
# condition's failure past the `if`, it's not just the shell's own
# reporting of the compound statement. Note rc's internal $status for
# a failed `~` is the descriptive string "no match", not a numeric
# code -- it's only turned into a numeric process exit status (what
# mk actually observes) when rc itself exits with that $status. ---
run_status_test "false cond leaves \$status set for later commands too" \
    'if(~ a b) echo yes
echo after:$status' "after:no match" "0"

# --- Test 4: the actual fix pattern used in the mkfiles -- adding
# `if not true` gives the false branch an explicit, successful last
# command, so overall status is 0 regardless of which branch fires. ---
run_status_test "if/if not true -> status 0 on the false branch" \
    'if(~ a b) echo yes
if not true' "" "0"

# --- Test 5: same fix pattern, true branch -- still status 0, and the
# real command still runs normally. ---
run_status_test "if/if not true -> status 0 on the true branch too" \
    'if(~ a a) echo yes
if not true' "yes" "0"

# --- Test 6: reproduces the exact hello_macos_arm64.exe recipe shape
# (backquote command substitution feeding the condition, `if` as the
# last statement) to pin the real-world failure mode, not just a
# synthetic `~` comparison. ---
run_status_test "mkfile-shaped: if(~ \`{cmd} Darwin) as last stmt fails on non-Darwin" \
    'if(~ `{echo Linux} Darwin) echo would-codesign' "" "1"

run_status_test "mkfile-shaped: same, with if not true, succeeds" \
    'if(~ `{echo Linux} Darwin) echo would-codesign
if not true' "" "0"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
