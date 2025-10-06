#include <u.h>
#include <libc.h>

char*
get9root(void)
{
	static char *s;

	if(s)
		return s;
    //goken: was "PLAN9" before
	if((s = getenv("GOROOT")) != 0)
		return s;
	s = PLAN9_TARGET;
	return s;
}
