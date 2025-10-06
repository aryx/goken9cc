#include <u.h>
#include <libc.h>

int
atoi(char *s)
{
	return strtol(s, 0, 0);
}

long
atol(char *s)
{
	return strtol(s, 0, 0);
}

vlong
atoll(char *s)
{
	return strtoll(s, 0, 0);
}
