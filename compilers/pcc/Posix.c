#include <lib9.h>
#include <bio.h>
#include <sys/wait.h>

void*
mysbrk(ulong size)
{
	return (void*)sbrk(size);
}

int
mycreat(char *n, int p)
{

	return create(n, 1, p);
}

int
mywait(int *s)
{
	return wait(s);
}

int
mydup(int f1, int f2)
{
	return dup2(f1,f2);
}

int
mypipe(int *fd)
{
	return pipe(fd);
}

char*
mygetwd(char *path, int len)
{
	return (char*)getcwd(path, len);
}

int
myexec(char *path, char *argv[])
{
	return execvp(path, argv);
}


int
myfork(void)
{
	return fork();
}

int
myaccess(char *f)
{
	return access(f, 0);
}
