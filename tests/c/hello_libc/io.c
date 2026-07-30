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

	close(fd);
	exit(0);
}
