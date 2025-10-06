#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include <sys/stat.h>

extern int _p9dir(struct stat*, struct stat*, char*, Dir*, char**, char*);

Dir*
dirstat(char *file)
{
	struct stat lst;
	struct stat st;
	int nstr;
	Dir *d;
	char *str;

#ifdef _WIN32
	if(stat(file, &st) < 0)
		return nil;
	lst = st;
#else
	if(lstat(file, &lst) < 0)
		return nil;
	st = lst;
	if((lst.st_mode&S_IFMT) == S_IFLNK)
		stat(file, &st);
#endif

	nstr = _p9dir(&lst, &st, file, nil, nil, nil);
	d = malloc(sizeof(Dir)+nstr);
	if(d == nil)
		return nil;
	memset(d, 0, sizeof(Dir)+nstr);
	str = (char*)&d[1];
	_p9dir(&lst, &st, file, d, &str, str+nstr);
	return d;
}

