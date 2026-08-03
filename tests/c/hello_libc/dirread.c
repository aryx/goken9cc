#include <u.h>
#include <libc.h>

/* dirread()/dirreadall() (include/os/dir.h) -- Tier 3.5 of
 * docs/claude_notes/plan_syscalls.txt, the piece of the stat family
 * that lists a directory's contents rather than stat-ing one already-
 * named entry (stat.c's job). Not built for every GOOS the way dir.c/
 * stat.c are -- see this file's own mkfile rules: darwin and windows
 * have no dirread() implementation at all yet (todo.org's own gap
 * entry), only linux (every arch except riscv, same reason stat.exe
 * skips it -- no fstat there either) and plan9 (built, not run --
 * same 5i/vi FSTAT gap stat.exe's plan9 build already documents).
 *
 * Fixture: a subdirectory with two known files and nothing else, so
 * the count and the set of names are fully deterministic -- unlike
 * proc.c's timestamps, nothing here varies per run or per machine.
 * Names are checked as a SET (order is not part of Plan9's dirread()
 * contract, and this test should not assume the kernel/emulator
 * returns entries in creation order).
 */
void
main(void)
{
	fdt fd;
	Dir *d;
	long n, i;
	int sawa, sawb, sawsub;

	remove("dirread_tmp_d/a.txt");
	remove("dirread_tmp_d/b.txt");
	remove("dirread_tmp_d/sub");
	remove("dirread_tmp_d");

	if (create("dirread_tmp_d", OREAD, DMDIR|0755) < 0) {
		print("create dir failed\n");
		exit(1);
	}
	if (create("dirread_tmp_d/sub", OREAD, DMDIR|0755) < 0) {
		print("create subdir failed\n");
		exit(1);
	}
	fd = create("dirread_tmp_d/a.txt", OWRITE, 0644);
	if (fd < 0) {
		print("create a.txt failed\n");
		exit(1);
	}
	write(fd, "aaa\n", 4);
	close(fd);
	fd = create("dirread_tmp_d/b.txt", OWRITE, 0644);
	if (fd < 0) {
		print("create b.txt failed\n");
		exit(1);
	}
	write(fd, "bb\n", 3);
	close(fd);

	fd = open("dirread_tmp_d", OREAD);
	if (fd < 0) {
		print("open dir failed\n");
		exit(1);
	}
	n = dirreadall(fd, &d);
	close(fd);
	if (n < 0) {
		print("dirreadall failed\n");
		exit(1);
	}
	if (n != 3) {
		print("dirreadall: expected 3 entries, got %ld\n", n);
		exit(1);
	}

	sawa = sawb = sawsub = 0;
	for (i = 0; i < n; i++) {
		/* "." and ".." must never appear -- a stubbed-out or
		 * naive implementation forgetting to skip them would
		 * report 5 entries instead of 3, already caught above,
		 * but check explicitly too for a clearer failure message.
		 */
		if (strcmp(d[i].name, ".") == 0 || strcmp(d[i].name, "..") == 0) {
			print("dirreadall: leaked %s\n", d[i].name);
			exit(1);
		}
		if (strcmp(d[i].name, "a.txt") == 0) {
			sawa = 1;
			if (d[i].length != 4) {
				print("a.txt: wrong length\n");
				exit(1);
			}
			if (d[i].mode & DMDIR) {
				print("a.txt: reported as a directory\n");
				exit(1);
			}
		} else if (strcmp(d[i].name, "b.txt") == 0) {
			sawb = 1;
			if (d[i].length != 3) {
				print("b.txt: wrong length\n");
				exit(1);
			}
		} else if (strcmp(d[i].name, "sub") == 0) {
			sawsub = 1;
			if (!(d[i].mode & DMDIR)) {
				print("sub: not reported as a directory\n");
				exit(1);
			}
		} else {
			print("dirreadall: unexpected entry %s\n", d[i].name);
			exit(1);
		}
	}
	if (!sawa || !sawb || !sawsub) {
		print("dirreadall: missing an expected entry\n");
		exit(1);
	}

	remove("dirread_tmp_d/sub");
	remove("dirread_tmp_d/a.txt");
	remove("dirread_tmp_d/b.txt");
	remove("dirread_tmp_d");
	print("dirread ok\n");
	exit(0);
}
