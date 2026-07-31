#include <u.h>
#include <libc.h>

/* access() and dup() -- the two calls that finish Tier 2 of
 * docs/claude_notes/plan_syscalls.txt.
 *
 * Kept apart from dir.c (and NOT built for GOOS=windows, see this
 * directory's mkfile) because dup() cannot be implemented honestly
 * there yet: os/windows/open.c's fd is a truncated HANDLE, so there is
 * no way to place a descriptor at a caller-chosen number. Every other
 * GOOS runs this.
 *
 * Both calls are interesting for the opposite reasons. access()'s
 * Plan9 mode bits (AEXIST/AEXEC/AWRITE/AREAD = 0/1/2/4) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so on Unix it is a bare syscall
 * -- but Plan9 itself has no access(2) at all and implements it over
 * open(). dup() is the mirror image: one Plan9 syscall covering what
 * POSIX splits into dup() and dup2(), selected by newfd == -1.
 */
void
main(void)
{
	fdt fd, fd2;
	char buf[64];
	long n;

	/* 1: access() on a file that exists, and on one that does not.
	 * Both directions matter -- an access() that always returned 0
	 * would pass the first check alone.
	 */
	if (access("io_input.txt", AEXIST) < 0) {
		print("access(AEXIST) failed on an existing file\n");
		exit(1);
	}
	if (access("io_input.txt", AREAD) < 0) {
		print("access(AREAD) failed on a readable file\n");
		exit(1);
	}
	if (access("no_such_file.txt", AEXIST) >= 0) {
		print("access succeeded on a missing file\n");
		exit(1);
	}
	print("access ok\n");

	/* 2: dup(fd, -1) -- "give me the lowest free descriptor". The copy
	 * must share the FILE OFFSET with the original, which is what
	 * distinguishes a real dup from merely opening the file twice: read
	 * 6 bytes through the original, then read through the copy and
	 * expect to continue where the first read stopped rather than
	 * starting over.
	 */
	fd = open("io_input.txt", OREAD);
	if (fd < 0) {
		print("open failed\n");
		exit(1);
	}
	n = read(fd, buf, 6);
	if (n != 6) {
		print("short read\n");
		exit(1);
	}
	fd2 = dup(fd, -1);
	if (fd2 < 0) {
		print("dup(fd, -1) failed\n");
		exit(1);
	}
	if (fd2 == fd) {
		print("dup returned the same fd\n");
		exit(1);
	}
	n = read(fd2, buf, 4);
	write(1, buf, n);
	write(1, "\n", 1);
	close(fd2);
	close(fd);

	/* 3: dup(fd, newfd) -- the dup2 half, placing the copy at a
	 * descriptor number WE choose. Picking a specific number is the
	 * whole point, so check we got exactly it back.
	 */
	fd = open("io_input.txt", OREAD);
	if (fd < 0) {
		print("reopen failed\n");
		exit(1);
	}
	fd2 = dup(fd, 9);
	if (fd2 != 9) {
		print("dup(fd, 9) did not land on 9\n");
		exit(1);
	}
	n = read(9, buf, 5);
	write(1, buf, n);
	write(1, "\n", 1);
	close(9);
	close(fd);

	exit(0);
}
