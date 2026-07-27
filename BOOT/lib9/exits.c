#include <u.h>
#include <libc.h>

// equal to _exits() in lib9
void
exits(char *s)
{
	if(s == nil || *s == '\0')
		exit(0);
	exit(exitcode(s));
}
