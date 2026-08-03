#include <u.h>
#include <libc.h>

/* dirstat()/dirfstat()/dirwstat()/dirfwstat() (include/os/stat.h) --
 * Tier 3 of docs/claude_notes/plan_syscalls.txt.
 *
 * Only dirfstat()/dirfwstat() (by fd) are implemented per GOOS/arch;
 * dirstat()/dirwstat() (by path) are portable wrappers over those (see
 * port/dirstat.c, port/dirwstat.c), so exercising the fd-based pair
 * plus one dirstat() call covers everything actually GOOS-specific.
 *
 * mode/length are checked with exact values (unlike proc.c's
 * timestamps, which vary per run and per machine): the mode is one
 * this test itself set at create() time, and the length is the exact
 * byte count just written, so both are fully deterministic.
 *
 * dirfwstat()'s mode-change path is NOT built for windows -- see
 * os/windows/stat.c's own header comment: Win32 has no simple by-HANDLE
 * chmod equivalent, only a by-PATH one, which a bare fd/HANDLE here
 * cannot reach. Same shape of gap as fd.c's dup() exclusion. That is
 * why this file is not in test_windows's binary list below.
 */
void
main(void)
{
	fdt fd;
	Dir *d;

	/* 1: create a fixture and check dirfstat() against it directly --
	 * mode as given to create(), length as just written, and NOT a
	 * directory.
	 */
	fd = create("stat_tmp.txt", OWRITE, 0644);
	if (fd < 0) {
		print("create failed\n");
		exit(1);
	}
	if (write(fd, "hello stat\n", 11) != 11) {
		print("write failed\n");
		exit(1);
	}

	d = dirfstat(fd);
	if (d == nil) {
		print("dirfstat failed\n");
		exit(1);
	}
	if ((d->mode & 0777) != 0644) {
		print("dirfstat: wrong mode\n");
		exit(1);
	}
	if (d->mode & DMDIR) {
		print("dirfstat: file reported as a directory\n");
		exit(1);
	}
	if (d->length != 11) {
		print("dirfstat: wrong length\n");
		exit(1);
	}

	/* 2: dirfwstat() a new mode, and prove it stuck by dirfstat()-ing
	 * again -- a stubbed-out dirfwstat that silently did nothing would
	 * still pass a check that only looked at dirfwstat()'s return value.
	 */
	nulldir(d);
	d->mode = 0600;
	if (dirfwstat(fd, d) < 0) {
		print("dirfwstat failed\n");
		exit(1);
	}
	free(d);

	d = dirfstat(fd);
	if (d == nil) {
		print("dirfstat after dirfwstat failed\n");
		exit(1);
	}
	if ((d->mode & 0777) != 0600) {
		print("dirfwstat: mode did not change\n");
		exit(1);
	}
	free(d);
	close(fd);

	/* 3: dirstat() by path -- the portable wrapper over the same
	 * dirfstat() this test already exercised directly, this time
	 * proving the open()+dirfstat()+close() bridge in port/dirstat.c
	 * itself, not just the per-GOOS half.
	 */
	d = dirstat("stat_tmp.txt");
	if (d == nil) {
		print("dirstat failed\n");
		exit(1);
	}
	if (d->length != 11) {
		print("dirstat: wrong length\n");
		exit(1);
	}
	free(d);

	/* 4: dirstat() on a directory -- proves the DMDIR bit, the one
	 * dirfstat() field #1 above deliberately did NOT exercise.
	 */
	d = dirstat(".");
	if (d == nil) {
		print("dirstat . failed\n");
		exit(1);
	}
	if (!(d->mode & DMDIR)) {
		print("dirstat: . not reported as a directory\n");
		exit(1);
	}
	free(d);

	remove("stat_tmp.txt");
	print("stat ok\n");
	exit(0);
}
