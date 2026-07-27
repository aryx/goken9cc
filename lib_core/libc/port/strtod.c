#include <u.h>
#include <libc.h>

/* fmt/strtod.c only defines fmtstrtod (used internally by dofmt.c's
 * number parsing); the plain ANSI name fmt/fltfmt.c's xdtoa() calls to
 * self-verify its own float-to-string conversion is expected to come
 * from elsewhere -- on the gcc/BOOT build that's glibc, here it's just
 * this thin wrapper.
 */
double
strtod(char *s, char **e)
{
	return fmtstrtod(s, e);
}
