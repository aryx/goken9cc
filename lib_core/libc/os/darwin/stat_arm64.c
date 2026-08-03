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

/* dirfstat()/dirfwstat() (include/os/stat.h) for darwin/arm64. Same
 * Kstat struct as stat_amd64.c (this GOOS's own sibling) -- see that
 * file's header comment for why the shape carries over unchanged even
 * though this arch calls a different syscall NUMBER (plain fstat, not
 * fstat64 -- numbers_arm64.h's own comment) to reach it.
 *
 * Unverified on a real macOS host -- see stat_amd64.c's identical note.
 */
typedef struct Kstat Kstat;
struct Kstat {
	int	dev;
	ushort	mode;
	ushort	nlink;
	uvlong	ino;
	uint	uid;
	uint	gid;
	int	rdev;
	uint	__pad0;
	vlong	atime;
	vlong	atime_nsec;
	vlong	mtime;
	vlong	mtime_nsec;
	vlong	ctime;
	vlong	ctime_nsec;
	vlong	btime;
	vlong	btime_nsec;
	vlong	size;
	vlong	blocks;
	int	blksize;
	uint	flags;
	uint	gen;
	int	lspare;
	vlong	qspare[2];
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
extern int _sysftruncate(int fd, vlong length);

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

/* claude: mode and length only -- see stat_amd64.c's (linux) dirfwstat()
 * comment: mtime-setting is a deliberate, documented gap on every arch.
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
		if (_sysftruncate(fd, d->length) < 0)
			ret = -1;
	}
	return ret;
}
