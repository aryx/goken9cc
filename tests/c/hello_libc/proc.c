#include <u.h>
#include <libc.h>

/* getpid(), getwd(), time()/nsec() and sleep() -- the "small tier".
 *
 * These are grouped because of what they have in common: each one is a
 * plain syscall on SOME systems and not a syscall at all on others, so
 * between them they exercise every shape of glue lib_core/libc has.
 *   getpid   syscall on linux/darwin; a file read on plan9 (#c/pid);
 *            a kernel32 call on windows.
 *   getwd    a real getcwd(2) only on linux. darwin and plan9 both lack
 *            one entirely and instead open "." and ask the kernel that
 *            descriptor's path -- fcntl(F_GETPATH) and fd2path, the
 *            same idea reached independently.
 *   time     clock_gettime on linux (the *_time64 forms on 32-bit, so
 *            the timespec is 8+8 everywhere), gettimeofday on darwin,
 *            /dev/bintime on plan9, a FILETIME epoch shift on windows.
 *   sleep    milliseconds, per include/os/time.h -- NOT POSIX seconds.
 *            A real syscall on plan9 and a plain Sleep() on windows,
 *            both needing no conversion; built from a timespec on linux
 *            and from select() on darwin, which has no nanosleep.
 *
 * Nothing here prints a VALUE, deliberately: pid, cwd and the clock all
 * differ per run and per machine, so the expected output has to be
 * value-independent. What is asserted instead are the invariants that
 * would actually break if the glue were wrong -- see each check.
 */

void
main(void)
{
	char buf[512];
	char *p;
	int pid, pid2;
	long t1, t2, tv;
	vlong n1, n2;

	/* 1: getpid. Can't check the value, but it must be positive and it
	 * must be STABLE -- a getpid that reopened and misparsed #c/pid, or
	 * returned a truncated handle, would plausibly return garbage that
	 * differs between two calls.
	 */
	pid = getpid();
	pid2 = getpid();
	if (pid <= 0) {
		print("getpid returned a non-positive pid\n");
		exit(1);
	}
	if (pid != pid2) {
		print("getpid returned two different values\n");
		exit(1);
	}

	/* 2: getwd. Must return the buffer it was handed (not a pointer to
	 * somewhere else), and must fill it with an absolute path. The
	 * leading-separator check is what catches a getcwd whose return
	 * value was misread as a length and left the buffer untouched.
	 */
	p = getwd(buf, sizeof buf);
	if (p == nil) {
		print("getwd failed\n");
		exit(1);
	}
	if (p != buf) {
		print("getwd did not return its own buffer\n");
		exit(1);
	}
	if (buf[0] != '/' && buf[0] != '\\' && buf[1] != ':') {
		print("getwd did not return an absolute path\n");
		exit(1);
	}

	/* and a buffer far too small must FAIL rather than overflow. This
	 * is the one error path here worth asserting: unlike brk (see
	 * mem.c), it is decided by libc and the kernel agreeing on a length,
	 * not by an emulator's whim, so it behaves the same everywhere.
	 */
	if (getwd(buf, 1) != nil) {
		print("getwd succeeded with a 1-byte buffer\n");
		exit(1);
	}

	/* 3: time() and nsec() must agree with each other. This is the
	 * check that catches a wrong epoch or a wrong scale factor -- the
	 * windows FILETIME shift and the linux 32-vs-64-bit timespec are
	 * both invisible to any test that merely calls the clock and looks
	 * for a nonzero answer. A clock off by centuries still passes that;
	 * it cannot pass this.
	 */
	t1 = time(nil);
	n1 = nsec();
	if (t1 <= 0) {
		print("time returned a non-positive value\n");
		exit(1);
	}
	if (n1 <= 0) {
		print("nsec returned a non-positive value\n");
		exit(1);
	}
	if (n1 / 1000000000LL - (vlong)t1 > 2 ||
	    (vlong)t1 - n1 / 1000000000LL > 2) {
		print("time and nsec disagree\n");
		exit(1);
	}

	/* time(&tv) must store the same value it returns -- the out
	 * parameter is easy to forget, and nothing else would notice.
	 */
	tv = 0;
	t2 = time(&tv);
	if (tv != t2) {
		print("time did not store through its argument\n");
		exit(1);
	}

	/* 4: sleep. Must actually advance the clock, and by roughly the
	 * amount asked -- 200ms, measured with nsec(). The lower bound is
	 * what catches a sleep that returned immediately; the generous
	 * upper bound (5s) still catches the classic milliseconds-vs-seconds
	 * error, which would sleep 200 SECONDS here.
	 */
	n1 = nsec();
	if (sleep(200) < 0) {
		print("sleep failed\n");
		exit(1);
	}
	n2 = nsec();
	if (n2 - n1 < 100000000LL) {
		print("sleep returned too early\n");
		exit(1);
	}
	if (n2 - n1 > 5000000000LL) {
		print("sleep slept far too long\n");
		exit(1);
	}

	print("proc ok\n");
	exit(0);
}
