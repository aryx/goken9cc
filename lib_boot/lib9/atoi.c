#include <u.h>
#include <libc.h>

int
atoi(char *s)
{
	return strtol(s, 0, 0);
}

//plan9port: in atol.c
long
atol(char *s)
{
	return strtol(s, 0, 0);
}

//plan9port: in atoll.c
vlong
atoll(char *s)
{
	return strtoll(s, 0, 0);
}
