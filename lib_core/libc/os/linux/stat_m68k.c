/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include <u.h>
#include <libc.h>

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/m68k. See
 * stat_amd64.c's header comment for the overall design.
 *
 * Struct layout is the kernel's own m68k `struct stat64` (what
 * SYS_fstat64 fills) -- confirmed against a real m68k-linux-any
 * kernel header found on this machine (see numbers_m68k.h's own
 * comment for the exact path; no m68k-linux-gnu cross-headers package
 * installed here). Same glibc2.1-style shape as stat_386.c's own
 * (double inode field: __st_ino a possibly-truncated 32-bit legacy
 * one nothing here reads, the real 64-bit inode trailing as `ino`,
 * used for qid.path below) but NOT byte-identical: this arch's own
 * __pad1/__pad3 are 2 bytes each, not 4 like 386's (two ushorts) --
 * get this wrong and every field from __st_ino onward is misaligned
 * by 2 bytes. st_blocks is also a real 8-byte `unsigned long long`
 * here (386's own Kstat already uses vlong for it too, so no
 * divergence there).
 */
typedef struct Kstat Kstat;
struct Kstat {
	uvlong	dev;
	uchar	__pad1[2];
	uint	__st_ino;
	uint	mode;
	uint	nlink;
	ulong	uid;
	ulong	gid;
	uvlong	rdev;
	uchar	__pad3[2];
	vlong	size;
	ulong	blksize;
	uvlong	blocks;
	ulong	atime;
	ulong	atime_nsec;
	ulong	mtime;
	ulong	mtime_nsec;
	ulong	ctime;
	ulong	ctime_nsec;
	uvlong	ino;
};

#define S_IFMT	0170000
#define S_IFDIR	0040000

static void
kstat2dir(Kstat *st, Dir *d)
{
	memset(d, 0, sizeof(Dir));
	d->name = d->uid = d->gid = d->muid = "";
	d->qid.path = st->ino;
	d->mode = st->mode & 0777;
	if ((st->mode & S_IFMT) == S_IFDIR) {
		d->mode |= DMDIR;
		d->qid.type = QTDIR;
	}
	d->atime = st->atime;
	d->mtime = st->mtime;
	d->length = st->size;
}

extern int _sysfstat(int fd, void *buf);
extern int _sysfchmod(int fd, int mode);
extern int _sysftruncate64(int fd, ulong lo, ulong hi);

Dir*
dirfstat(fdt fd)
{
	Kstat st;
	Dir *d;

	if (_sysfstat(fd, &st) < 0)
		return nil;
	d = malloc(sizeof(Dir));
	if (d == nil)
		return nil;
	kstat2dir(&st, d);
	return d;
}

/* claude: mode and length only -- see stat_amd64.c's dirfwstat()
 * comment: mtime-setting is a deliberate, documented gap. length uses
 * _sysftruncate64's pre-split lo/hi shape -- see
 * syscall_linux_386.decl's comment on why the kernel call itself takes
 * two words instead of one 64-bit argument.
 */
int
dirfwstat(fdt fd, Dir *d)
{
	int ret;

	ret = 0;
	if (~d->mode != 0) {
		if (_sysfchmod(fd, (int)(d->mode & 0777)) < 0)
			ret = -1;
	}
	if (~d->length != 0) {
		if (_sysftruncate64(fd, (ulong)d->length,
		    (ulong)((uvlong)d->length >> 32)) < 0)
			ret = -1;
	}
	return ret;
}

/* claude: dirread()/dirreadall() (Tier 3.5) -- see stat_386.c's
 * identical comment for the full design (getdents64 + openat +
 * dirfstat, "."/".." skipped, one dirread() call = one getdents64
 * buffer's worth, dirreadall() loops to EOF).
 */
typedef struct Dirent64 Dirent64;
struct Dirent64 {
	uvlong	ino;
	vlong	off;
	ushort	reclen;
	uchar	type;
	char	name[1];
};

extern long openat(int dirfd, void *path, int flags, int mode);
extern long _sysgetdents64(int fd, void *buf, uint count);

static long
dirreadbuf(fdt fd, Dir **dp, int all)
{
	uchar buf[8192];
	Dir *d, *nd, *tmp;
	long n, off, ndir, cap;
	fdt cfd;
	Dirent64 *de;

	d = nil;
	ndir = 0;
	cap = 0;
	for (;;) {
		n = _sysgetdents64(fd, buf, sizeof buf);
		if (n <= 0)
			break;
		for (off = 0; off < n; off += de->reclen) {
			de = (Dirent64*)(buf + off);
			if (de->name[0] == '.' && (de->name[1] == 0 ||
			    (de->name[1] == '.' && de->name[2] == 0)))
				continue;
			cfd = openat(fd, de->name, 0, 0);
			if (cfd < 0)
				continue;
			nd = dirfstat(cfd);
			close(cfd);
			if (nd == nil)
				continue;
			nd->name = strdup(de->name);
			if (ndir >= cap) {
				cap = cap ? cap*2 : 16;
				tmp = realloc(d, cap * sizeof(Dir));
				if (tmp == nil) {
					free(nd);
					free(d);
					return -1;
				}
				d = tmp;
			}
			d[ndir++] = *nd;
			free(nd);
		}
		if (!all)
			break;
	}
	*dp = d;
	return ndir;
}

long
dirread(fdt fd, Dir **dp)
{
	return dirreadbuf(fd, dp, 0);
}

long
dirreadall(fdt fd, Dir **dp)
{
	return dirreadbuf(fd, dp, 1);
}
