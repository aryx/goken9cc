#!/bin/bash
# Test that rc's glob handles directories containing long filenames.
#
# Bug: NDIR=14 in unix.c (from V7 Unix 14-char limit) makes Globsize()
# allocate a buffer too small for modern filenames (up to 255 chars).
# When globdir() iterates directory entries, Readdir() does strcpy()
# into this undersized buffer, causing heap-buffer-overflow.
#
# Reproduction: create a file whose name exceeds the buffer size for a
# given glob pattern, then run rc with that glob in the same directory.
# With NDIR=14, the buffer for "[xy].out" is only 23 bytes, so any
# filename >= 24 chars overflows it.

set -e

RC=${RC:-rc}
PASS=0
FAIL=0

run_glob_test() {
    local name="$1"
    local dir="$2"
    local cmd="$3"
    local expected="$4"

    # Run rc and capture output + exit status
    local actual status
    actual=$(cd "$dir" && "$RC" -c "$cmd" 2>/dev/null) && status=$? || status=$?

    if [ -n "$expected" ] && [ "$actual" != "$expected" ]; then
        echo "FAIL: $name (expected '$expected', got '$actual', exit $status)"
        FAIL=$((FAIL + 1))
    elif [ $status -ne 0 ] && [ -z "$expected" ] && [ -n "$actual" ]; then
        echo "FAIL: $name (unexpected output '$actual', exit $status)"
        FAIL=$((FAIL + 1))
    else
        echo "PASS: $name"
        PASS=$((PASS + 1))
    fi
}

TESTBASE=$(mktemp -d)
trap 'rm -rf "$TESTBASE"' EXIT

# --- Test 1: bracket glob in dir with a long filename ---
DIR1="$TESTBASE/t1"
mkdir "$DIR1"
# 50-char filename: well beyond the 23-byte buffer for [xy].out
touch "$DIR1/this_is_a_filename_longer_than_twenty_three_chars"
run_glob_test "bracket glob with 50-char file in dir" \
    "$DIR1" "echo [xy].out" "[xy].out"

# --- Test 2: star glob in dir with a long filename ---
DIR2="$TESTBASE/t2"
mkdir "$DIR2"
touch "$DIR2/another_very_long_filename_exceeding_the_buffer_sz"
touch "$DIR2/short.c"
run_glob_test "star glob with long file in dir" \
    "$DIR2" "echo *.c" "short.c"

# --- Test 3: reproduce the actual mk clean pattern that crashes ---
DIR3="$TESTBASE/t3"
mkdir "$DIR3"
# These are real filenames from lib_core/libc/9sys/ that triggered the
# original crash in principia-softwarica's mk clean
for f in \
    getnetconninfo.c setnetmtpt.c dirmodefmt.c privalloc.c \
    read9pmsg.c dirfwstat.c dirfstat.c announce.c fcallfmt.c \
    sysfatal.c truerand.c sysname.c waitpid.c werrstr.c rerrstr.c \
    cputime.c nulldir.c pushssl.c pushtls.c getenv.c getpid.c \
    getppid.c syslog.c access.c putenv.c abort.c ctime.c dial.c \
    fork.c nsec.c qlock.c read.c readv.c sbrk.c time.c times.c \
    wait.c write.c writev.c getwd.c iounit.c tm2sec.c dirread.c \
    dirstat.c dirwstat.c postnote.c mkfile
do
    touch "$DIR3/$f"
done
# Add a file long enough to guarantee overflow of the 23-byte buffer
touch "$DIR3/a_long_name_for_testing_glob_overflow.c"
run_glob_test "mk clean pattern with long filenames" \
    "$DIR3" "rm -f *.[58] [58].out" ""

# --- Test 4: NAME_MAX-length filename (255 chars) ---
DIR4="$TESTBASE/t4"
mkdir "$DIR4"
LONG=$(python3 -c "print('x' * 255)")
touch "$DIR4/$LONG"
run_glob_test "bracket glob with NAME_MAX filename" \
    "$DIR4" "echo [ab].txt" "[ab].txt"

# --- Test 5: glob that actually matches a long filename ---
DIR5="$TESTBASE/t5"
mkdir "$DIR5"
touch "$DIR5/a_file_with_a_really_long_name_for_testing.txt"
run_glob_test "star glob matching long filename" \
    "$DIR5" "echo a_file_*" "a_file_with_a_really_long_name_for_testing.txt"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
