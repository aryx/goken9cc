#include <u.h>
#include <libc.h>

void
_exits(char *s)
{
	if(s == nil || *s == '\0')
		_exit(0);
	_exit(exitcode(s));
}
