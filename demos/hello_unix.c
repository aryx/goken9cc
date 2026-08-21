#include <u.h>
#include <libc.h>

/* A guided tour of lib_core/libc, run through goken's own compiler,
 * assembler and linker for whichever $objtype/$GOOS this was built
 * for. Where tests/c/hello_libc/*.c each isolate ONE syscall family
 * and assert an exact expected.txt, this prints a short human-readable
 * report of each in turn -- meant to be read by a person running
 * `mk demo`, not diffed by a script. Only functions that build (and,
 * on Linux/qemu and Plan9/5i-vi, actually run) on every arch that
 * demos/mkfile's ARCHS loop covers are used here: args, getenv, getpid/
 * getwd/time, sbrk, create/write/read/remove (file and DMDIR), ctime,
 * atexit. See tests/c/hello_libc/mkfile's own per-file comments for
 * which of these are load-bearing on which GOOS.
 */

enum
{
	/* claude: kept under 128 deliberately -- see
	 * docs/claude_notes/notes_shared_frontend_bugs.txt's "Equality
	 * comparison of a high-bit char against a runtime-computed cast"
	 * entry (tests/c/regressions/char_cmp_high_bit_ic_qc.c): ic/qc
	 * miscompare a `char` >= 128 against a runtime-cast expression,
	 * which would make this demo's own readback check falsely FAIL on
	 * riscv/power even though the memory is written correctly.
	 */
	SBRKN	= 100
};

int order[2];
int norder;

void
atexitfirst(void)
{
	order[norder++] = 1;
}

void
atexitsecond(void)
{
	order[norder++] = 2;
}

void
atexitreport(void)
{
	if (norder == 2 && order[0] == 2 && order[1] == 1)
		print("atexit: two handlers ran in LIFO order -- ok\n");
	else
		print("atexit: WRONG order (n=%d order[0]=%d order[1]=%d)\n",
			norder, order[0], order[1]);
}

void
main(int argc, char *argv[])
{
	char *v;
	char buf[512];
	char *p1;
	fdt fd;
	long n;
	int i, pid;
	long t;

	print("=== goken hello_unix demo ===\n");

	/* args: proves arch/$cputype/rt0.s's argc/argv bridge is wired up
	 * for this build, not just that main() runs at all.
	 */
	print("args:  argc=%d", argc);
	for (i = 1; i < argc; i++)
		print(" argv[%d]=%s", i, argv[i]);
	print("\n");

	/* env: the environment reaches main() through a completely
	 * different path per GOOS -- see env.c's own header comment.
	 */
	v = getenv("PATH");
	if (v != nil)
		print("env:   PATH is set (%d bytes)\n", strlen(v));
	else
		print("env:   PATH not visible on this GOOS\n");

	/* proc: pid/cwd/clock -- three more calls, three more shapes of
	 * glue underneath (real syscall, file read, kernel32), see proc.c.
	 */
	pid = getpid();
	p1 = getwd(buf, sizeof buf);
	print("proc:  pid=%d cwd=%s\n", pid, p1 != nil ? p1 : "<unavailable>");

	/* mem: sbrk-backed heap growth -- write a pattern into fresh
	 * memory and read it back, proving the returned range is real,
	 * mapped memory and not just a plausible-looking pointer.
	 */
	p1 = sbrk(SBRKN);
	if (p1 == (char*)-1) {
		print("mem:   sbrk failed\n");
	} else {
		for (i = 0; i < SBRKN; i++)
			p1[i] = (char)(i + 1);
		for (i = 0; i < SBRKN; i++)
			if (p1[i] != (char)(i + 1))
				break;
		print("mem:   sbrk(%d) -- write+readback %s\n", SBRKN,
			i == SBRKN ? "ok" : "FAILED");
	}

	/* file: create()/write()/read-back()/remove() -- the calls that
	 * modify the file system, see dir.c's own header comment on why
	 * create isn't a syscall at all on Unix (open(O_CREAT) underneath)
	 * but is one directly on Plan9.
	 */
	fd = create("hello_unix_tmp.txt", OWRITE, 0666);
	if (fd < 0) {
		print("file:  create failed\n");
	} else {
		write(fd, "Hello from goken!\n", 19);
		close(fd);
		fd = open("hello_unix_tmp.txt", OREAD);
		n = fd >= 0 ? read(fd, buf, sizeof(buf) - 1) : -1;
		if (fd >= 0)
			close(fd);
		if (n >= 0)
			buf[n] = 0;
		remove("hello_unix_tmp.txt");
		print("file:  wrote+read back %s: %s", n >= 0 ? "ok" : "FAILED",
			n >= 0 ? buf : "\n");
	}

	/* dir: the DMDIR path -- Plan9 has no mkdir(), a directory is a
	 * create() with DMDIR set in the permission bits (see dir.c).
	 */
	fd = create("hello_unix_tmp_d", OREAD, DMDIR|0777);
	if (fd < 0) {
		print("dir:   create DMDIR failed\n");
	} else {
		close(fd);
		if (remove("hello_unix_tmp_d") < 0)
			print("dir:   created hello_unix_tmp_d/ but remove FAILED\n");
		else
			print("dir:   created and removed hello_unix_tmp_d/ -- ok\n");
	}

	/* ctime: pure calendar arithmetic (port/ctime.c), same on every
	 * GOOS/arch -- print the fixed Unix epoch plus this run's wall
	 * clock, formatted through asctime()/ctime().
	 */
	print("ctime: epoch 0 is %s", ctime(0));
	t = time(nil);
	if (t > 0)
		print("ctime: right now is %s", ctime(t));

	/* atexit: LIFO-ordered callbacks, run from exits() -- see
	 * port/atexit.c/exits.c. Registered last so it reports after
	 * everything above has already printed.
	 */
	atexit(atexitreport);
	atexit(atexitfirst);
	atexit(atexitsecond);

	print("=== demo complete ===\n");
	exits(nil);
}
