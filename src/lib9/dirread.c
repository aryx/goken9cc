#define _GNU_SOURCE	/* for Linux O_DIRECT */
#include <u.h>
#include <dirent.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#define NOPLAN9DEFINES
#include <libc.h>

static struct {
	Lock lk;
	DIR **d;
	int nd;
} dirs;

static DIR*
dirget(int fd)
{
	DIR *d;

	lock(&dirs.lk);
	d = nil;
	if(0 <= fd && fd < dirs.nd)
		d = dirs.d[fd];
	unlock(&dirs.lk);
	return d;
}

extern int _p9dir(struct stat*, struct stat*, char*, Dir*, char**, char*);

typedef struct DirBuild DirBuild;
struct DirBuild {
	Dir *d;
	int nd;
	int md;
	char *str;
	char *estr;
};


static int
dirbuild1(DirBuild *b, struct stat *lst, struct stat *st, char *name)
{
	int i, nstr;
	Dir *d;
	int md, mstr;
	char *lo, *hi, *newlo;

	nstr = _p9dir(lst, st, name, nil, nil, nil);
	if(b->md-b->nd < 1 || b->estr-b->str < nstr) {
		// expand either d space or str space or both.
		md = b->md;
		if(b->md-b->nd < 1) {
			md *= 2;
			if(md < 16)
				md = 16;
		}
		mstr = b->estr-(char*)&b->d[b->md];
		if(b->estr-b->str < nstr) {
			mstr += nstr;
			mstr += mstr/2;
		}
		if(mstr < 512)
			mstr = 512;
		d = realloc(b->d, md*sizeof d[0] + mstr);
		if(d == nil)
			return -1;
		// move strings and update pointers in Dirs
		lo = (char*)&b->d[b->md];
		newlo = (char*)&d[md];
		hi = b->str;
		memmove(newlo, lo+((char*)d-(char*)b->d), hi-lo);
		for(i=0; i<b->nd; i++) {
			if(lo <= d[i].name && d[i].name < hi)
				d[i].name += newlo - lo;
			if(lo <= d[i].uid && d[i].uid < hi)
				d[i].uid += newlo - lo;
			if(lo <= d[i].gid && d[i].gid < hi)
				d[i].gid += newlo - lo;
			if(lo <= d[i].muid && d[i].muid < hi)
				d[i].muid += newlo - lo;
		}
		b->d = d;
		b->md = md;
		b->str += newlo - lo;
		b->estr = newlo + mstr;
	}
	_p9dir(lst, st, name, &b->d[b->nd], &b->str, b->estr);
	b->nd++;
	return 0;
}

static long
dirreadmax(int fd, Dir **dp, int max)
{
	int i;
	DIR *dir;
	DirBuild b;
	struct dirent *de;
	struct stat st, lst;

	if((dir = dirget(fd)) == nil) {
		werrstr("not a directory");
		return -1;
	}

	memset(&b, 0, sizeof b);
	for(i=0; max == -1 || i<max; i++) { // max = not too many, not too few
		errno = 0;
		de = readdir(dir);
		if(de == nil) {
			if(b.nd == 0 && errno != 0)
				return -1;
			break;
		}
		// Note: not all systems have d_namlen. Assume NUL-terminated.
		if(de->d_name[0]=='.' && de->d_name[1]==0)
			continue;
		if(de->d_name[0]=='.' && de->d_name[1]=='.' && de->d_name[2]==0)
			continue;
		if(fstatat(fd, de->d_name, &lst, AT_SYMLINK_NOFOLLOW) < 0)
			continue;
		st = lst;
		if(S_ISLNK(lst.st_mode))
			fstatat(fd, de->d_name, &st, 0);
		dirbuild1(&b, &lst, &st, de->d_name);
	}
	*dp = b.d;
	return b.nd;
}

long
dirread(int fd, Dir **dp)
{
	return dirreadmax(fd, dp, 10);
}

long
dirreadall(int fd, Dir **dp)
{
	return dirreadmax(fd, dp, -1);
}
