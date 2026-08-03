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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/arm64. See
 * stat_amd64.c's header comment for the overall design (portable
 * dirstat()/dirwstat() on top of these, no host headers).
 *
 * Struct layout is the Linux "generic" struct stat (asm-generic/stat.h
 * upstream -- arm64/riscv64 both use it, see stat_riscv64.c), confirmed
 * against this host's own /usr/include/asm-generic/stat.h (this box is
 * aarch64, so it resolves natively here). Every field that is a real
 * 8-byte `long`/`unsigned long` on this 64-bit arch is declared
 * vlong/uvlong, not this project's own 4-byte `long` -- see
 * stat_amd64.c's comment for why that distinction matters.
 */
typedef struct Kstat Kstat;
struct Kstat {
	uvlong	dev;
	uvlong	ino;
	uint	mode;
	uint	nlink;
	uint	uid;
	uint	gid;
	uvlong	rdev;
	uvlong	__pad1;
	vlong	size;
	int	blksize;
	int	__pad2;
	vlong	blocks;
	vlong	atime;
	uvlong	atime_nsec;
	vlong	mtime;
	uvlong	mtime_nsec;
	vlong	ctime;
	uvlong	ctime_nsec;
	uint	__unused4;
	uint	__unused5;
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

/* claude: mode and length only -- see stat_amd64.c's dirfwstat()
 * comment: mtime-setting is a deliberate, documented gap.
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
