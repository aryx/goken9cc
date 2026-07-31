#include <u.h>
#include <libc.h>

/* Beyond hello.c's plain print()/exit(): exercises open/read/seek/write/
 * close on a real file (io_input.txt, this directory) instead of just
 * stdout. Plan9's open() (include/os/file.h) never creates files -- no
 * OCREATE flag exists in that API at all -- so this reads a pre-existing
 * fixture rather than writing a new one.
 */
void
main(void)
{
	fdt fd;
	char buf[256];
	long n;
	vlong off;

	fd = open("io_input.txt", OREAD);
	if (fd < 0) {
		print("open failed\n");
		exit(1);
	}

	/* 1: print the whole fixture once */
	n = read(fd, buf, sizeof(buf));
	write(1, buf, n);

	/* 2: seek back to the start and re-read just a prefix, to prove
	 * seek() actually moved the file offset rather than read() always
	 * starting from 0
	 */
	seek(fd, 0, 0);
	n = read(fd, buf, 5);
	write(1, buf, n);
	write(1, "\n", 1);

	/* 3: a NONZERO seek, checking seek()'s own RETURN VALUE and not
	 * just the bytes that come back. Both halves matter, and neither is
	 * covered by step 2 above: offset 0 is precisely the value that
	 * survives being passed in the wrong argument slot, or truncated,
	 * or ignored -- so a seek() that is completely misassembled can
	 * still make step 2 pass. This step was added after exactly such a
	 * bug was found on riscv32, where syscall 62 is llseek(fd,
	 * offset_high, offset_low, loff_t *result, whence) rather than
	 * rv64's lseek(fd, offset, whence): the arguments did not line up
	 * at all and the result pointer was NULL, yet seek(fd, 0, 0) still
	 * "worked" because the kernel computed the right zero offset before
	 * failing to write it back. Checking the returned offset is what
	 * catches that, since it comes back through the pointer llseek
	 * fills in. See docs/claude_notes/notes_arch_riscv.txt.
	 */
	off = seek(fd, 6, 0);
	if (off != 6) {
		print("seek returned wrong offset\n");
		exit(1);
	}
	n = read(fd, buf, 4);
	write(1, buf, n);
	write(1, "\n", 1);

	close(fd);
	exit(0);
}
