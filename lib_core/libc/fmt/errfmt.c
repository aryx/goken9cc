#include <u.h>
#include <libc.h>
#include "fmtdef.h"

int
__errfmt(Fmt *f)
{
	char *s;

	s = strerror(errno);
	return fmtstrcpy(f, s);
}
