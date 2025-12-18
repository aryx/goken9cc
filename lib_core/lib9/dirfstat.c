#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include <sys/stat.h>

extern int _p9dir(struct stat*, struct stat*, char*, Dir*, char**, char*);

Dir*
dirfstat(int fd)
{
	struct stat st;
	int nstr;
	Dir *d;
	char *str, tmp[100];

	if(fstat(fd, &st) < 0)
		return nil;

	snprint(tmp, sizeof tmp, "/dev/fd/%d", fd);
	nstr = _p9dir(&st, &st, tmp, nil, nil, nil);
	d = malloc(sizeof(Dir)+nstr);
	if(d == nil)
		return nil;
	memset(d, 0, sizeof(Dir)+nstr);
	str = (char*)&d[1];
	_p9dir(&st, &st, tmp, d, &str, str+nstr);
	return d;
}

