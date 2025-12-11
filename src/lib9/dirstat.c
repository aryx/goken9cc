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

    // the stat() call! (actually lstat)
	if(lstat(file, &lst) < 0)
		return nil;
	st = lst;
	if((lst.st_mode&S_IFMT) == S_IFLNK)
		stat(file, &st);

    // call1 with nils
	nstr = _p9dir(&lst, &st, file, nil, nil, nil);

    // reserve extra space after Dir for name of entry for ?? debugging??
	d = malloc(sizeof(Dir)+nstr);
	if(d == nil)
		return nil;
	memset(d, 0, sizeof(Dir)+nstr);
    // str points after d
	str = (char*)&d[1];

    // call2
	_p9dir(&lst, &st, file, d, &str, str+nstr);
	return d;
}

