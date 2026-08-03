#include <u.h>
#include <libc.h>

/* getenv() -- the fourth distinct way this libc reaches the operating
 * system, after syscalls (io.c), file reads (proc.c's plan9 path) and
 * kernel32 calls.
 *
 * On linux and darwin it is not a syscall or a call of any kind: the
 * environment is memory the kernel already laid on the initial stack,
 * immediately past argv's terminating nil, so port/getenv.c just walks
 * it (using the argv arch/$cputype/rt0.s stashes -- see
 * port/mainargs.c). On plan9 the environment is a DIRECTORY, /env, one
 * file per variable. On windows it is a kernel32 call.
 *
 * It tests against $PATH rather than a variable the mkfile sets for the
 * occasion, which was the first instinct. That does not work here: this
 * project's own rc has a no-op Updenv() (rc/unix.c -- the POSIX
 * environ-rebuilding half is commented out, unlike rc/plan9.c's real
 * one), so "VAR=val cmd" in an mk recipe never reaches the child at
 * all. Verified directly rather than assumed. So the test uses
 * something the process already has, and leans on NAME MATCHING rather
 * than on any particular value for its sharp edges -- checks 2 through
 * 5 below are the ones that would actually catch a broken getenv, and
 * none of them depends on what PATH contains.
 */

void
main(void)
{
	char *v;

	/* 1: PATH must be found, and what comes back must be the VALUE, not
	 * the whole "PATH=..." entry and not a pointer at the '='. Testing
	 * the first character catches both of those off-by-ones without
	 * asserting anything machine-specific: every PATH entry is an
	 * absolute path or empty, so it never begins with '=' or 'P'.
	 */
	v = getenv("PATH");
	if (v == nil) {
		print("getenv did not find PATH\n");
		exit(1);
	}
	if (v[0] == '=' || v[0] == '\0') {
		print("getenv returned the wrong part of the entry\n");
		exit(1);
	}

	/* 2: a name that does not exist must return nil. An implementation
	 * that walked off the end of the environment array would more
	 * likely crash here than return anything.
	 */
	if (getenv("GOKEN_ENV_ABSENT_XYZZY") != nil) {
		print("getenv found a variable that does not exist\n");
		exit(1);
	}

	/* 3: a strict PREFIX of a real variable must not match. This is the
	 * single most likely bug in any getenv -- comparing only strlen(name)
	 * bytes and forgetting to require '=' next -- and "PAT" exercises it
	 * against a variable that is certainly present.
	 */
	if (getenv("PAT") != nil) {
		print("getenv matched a strict prefix\n");
		exit(1);
	}

	/* 4: and the reverse direction -- a name of which a real variable is
	 * a prefix must not match either.
	 */
	if (getenv("PATHZZZ") != nil) {
		print("getenv matched an over-long name\n");
		exit(1);
	}

	/* 5: degenerate input must not crash. */
	if (getenv("") != nil) {
		print("getenv matched the empty name\n");
		exit(1);
	}

	print("env ok\n");
	exit(0);
}
