#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include <sys/stat.h>

bool
fileexists(char *file)
{
	struct stat st;

    return stat(file, &st) == OK_0;
}

