#include <u.h>
#include <libc.h>

// for "#9/..." paths in unsharp.c
char*
get9root(void)
{
	static char *s;

	if(s)
		return s;
    //goken: was "PLAN9" before
	if((s = getenv("GOROOT")) != nil)
		return s;
    //goken: was PLAN9_TARGET
	s = GOROOT;
	return s;
}
