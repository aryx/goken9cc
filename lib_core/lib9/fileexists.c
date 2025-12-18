#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include <sys/stat.h>

//pad: I added this file. This function was duplicated in many linkers
// before in kencc.

bool
fileexists(char *file)
{
	struct stat st;

    return stat(file, &st) == OK_0;
}

