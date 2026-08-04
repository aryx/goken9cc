#include <u.h>
#include <libc.h>

/* strrchr()/strncpy()/utfrrune()/dirmodefmt() -- small gaps found while
 * retrying utilities/files/ls.c after ctime() was implemented (see
 * docs/claude_notes/notes_libc_selfhost.txt): utfrrune() (used by
 * ls.c's xcleanname()) was declared+implemented but never wired into
 * PORTOFILES, and used strrchr() which didn't exist anywhere; %M's
 * dirmodefmt() didn't exist at all and its absence, uninstalled, made
 * ls -l desync its own varargs and segfault on a stale %q pointer (see
 * port/dirmodefmt.c's own header comment for how that was diagnosed).
 */
void
main(void)
{
	char buf[16];
	char *s;

	/* strrchr(): last occurrence, not first -- "a/b/c" has three
	 * slashes, must find the last one.
	 */
	s = strrchr("a/b/c", '/');
	if (s == nil || strcmp(s, "/c") != 0) {
		print("strrchr: wrong result\n");
		exit(1);
	}
	if (strrchr("abc", 'z') != nil) {
		print("strrchr: found a byte that isn't there\n");
		exit(1);
	}

	/* strncpy(): copies at most n bytes, zero-pads the rest of the
	 * destination when the source is shorter than n (the one behavior
	 * plain strcpy() doesn't have, and dirmodefmt()'s rwx() below
	 * relies on for buf[]'s NUL termination).
	 */
	memset(buf, 'x', sizeof buf);
	strncpy(buf, "hi", 5);
	if (memcmp(buf, "hi\0\0\0", 5) != 0) {
		print("strncpy: wrong padding\n");
		exit(1);
	}

	/* utfrrune(): same "last occurrence" shape as strrchr(), but over
	 * a UTF-8 rune instead of a single byte -- ls.c's xcleanname()
	 * uses it to find the last '/' in a path, so this is effectively
	 * the same check strrchr() above got, through the rune-aware path
	 * instead (c='/' is < Runesync, so utfrrune() itself dispatches
	 * straight to strrchr() -- see port/../utf/utfrrune.c).
	 */
	s = utfrrune("a/b/c", '/');
	if (s == nil || strcmp(s, "/c") != 0) {
		print("utfrrune: wrong result\n");
		exit(1);
	}

	/* dirmodefmt(): the %M fmtinstall() handler. A plain file (no
	 * DMDIR/DMAPPEND/DMAUTH/DMEXCL bits) with mode 0644 must format as
	 * "--rw-r--r--" -- Plan9's own 11-char shape (type + excl flag +
	 * 3x3 rwx), not the 10-char Unix "ls -l" one.
	 */
	fmtinstall('M', dirmodefmt);
	snprint(buf, sizeof buf, "%M", (ulong)0644);
	if (strcmp(buf, "--rw-r--r--") != 0) {
		print("dirmodefmt: wrong string: %s\n", buf);
		exit(1);
	}

	/* and again with DMDIR set, to exercise the 'd' branch. */
	snprint(buf, sizeof buf, "%M", (ulong)(DMDIR|0755));
	if (strcmp(buf, "d-rwxr-xr-x") != 0) {
		print("dirmodefmt: wrong string for a directory: %s\n", buf);
		exit(1);
	}

	print("utfmisc ok\n");
	exits(nil);
}
