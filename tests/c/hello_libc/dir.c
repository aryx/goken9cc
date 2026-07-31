#include <u.h>
#include <libc.h>

/* Beyond io.c's open/read/seek/write/close on a pre-existing fixture:
 * this one exercises the three calls that MODIFY the file system --
 * create(), remove() and chdir() (include/os/dir.h). Which is also why
 * io.c couldn't just be extended: it deliberately only ever reads, so
 * it can run from a read-only tree; this test needs a writable cwd.
 *
 * Each step is written so that a stubbed-out or silently-failing
 * implementation would be caught, rather than an fd or a 0 return being
 * taken at face value: the created file is reopened and read back, and
 * the removed one is reopened and expected to FAIL. The three calls
 * cover the three different ways this project has to implement them
 * (see lib_core/libc/syscall/os/): chdir is a real syscall of the same
 * name everywhere; remove is unlink(2) on the older Linux archs and
 * Darwin but a shim over unlinkat(2) on arm64/riscv/riscv64, which
 * dropped the non-*at forms; and create isn't a syscall at all on Unix,
 * it's open(O_CREAT|O_TRUNC) plus flag translation in os/$GOOS/open.c.
 * Only GOOS=plan9 gets all three as plain kernel calls.
 */
void
main(void)
{
	fdt fd;
	char buf[64];
	long n;

	/* 1: create a file that doesn't exist yet. Plan9's create() both
	 * makes the file and hands back an fd already open in the
	 * requested mode -- there's no separate creat()-then-open() dance.
	 */
	fd = create("dir_tmp.txt", OWRITE, 0666);
	if (fd < 0) {
		print("create failed\n");
		exit(1);
	}
	write(fd, "created\n", 8);
	close(fd);

	/* 2: reopen and read it back. This is what proves create() really
	 * put a file on disk with the right contents, rather than just
	 * returning a plausible-looking fd.
	 */
	fd = open("dir_tmp.txt", OREAD);
	if (fd < 0) {
		print("reopen after create failed\n");
		exit(1);
	}
	n = read(fd, buf, sizeof(buf));
	write(1, buf, n);
	close(fd);

	/* 3: remove it, then prove it's gone by requiring open() to fail.
	 * Without this second half, a remove() that returned 0 without
	 * doing anything would still pass.
	 */
	if (remove("dir_tmp.txt") < 0) {
		print("remove failed\n");
		exit(1);
	}
	if (open("dir_tmp.txt", OREAD) >= 0) {
		print("remove left the file behind\n");
		exit(1);
	}
	print("removed\n");

	/* 4: chdir to the parent, then open a path that can only resolve
	 * from there. io_input.txt is io.c's fixture and lives in THIS
	 * directory, so "hello_libc/io_input.txt" is findable only once
	 * the cwd has actually moved up a level -- a chdir() that returned
	 * 0 without moving anything would fail here.
	 */
	if (chdir("..") < 0) {
		print("chdir failed\n");
		exit(1);
	}
	fd = open("hello_libc/io_input.txt", OREAD);
	if (fd < 0) {
		print("chdir did not move the cwd\n");
		exit(1);
	}
	close(fd);
	print("chdir ok\n");

	exit(0);
}
