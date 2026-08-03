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

/* dirfstat()/dirfwstat() (include/os/stat.h) for darwin/amd64. See
 * lib_core/libc/os/linux/stat_amd64.c's header comment for the overall
 * design (portable dirstat()/dirwstat() on top of these, no host
 * headers, vlong/uvlong for every genuinely-8-byte kernel field rather
 * than this project's own 4-byte `long`).
 *
 * Struct layout is XNU's unified 64-bit stat64 struct, field-for-field
 * from GO/pkg/syscall/ztypes_darwin_amd64.go's Stat_t -- the same
 * 2010-era Go snapshot numbers_amd64.h's getwd/time comment already
 * flags as untrustworthy for the stat NUMBERS on this GOOS (it predates
 * arm64 macOS), but the STRUCT shape is fine to reuse: it is amd64's
 * post-unification stat64 layout, and per numbers_arm64.h's comment,
 * arm64 macOS's plain fstat already returns this exact same shape --
 * hence stat_arm64.c shares this file's Kstat verbatim.
 *
 * Unverified on a real macOS host, like every other os/darwin/*.c file
 * added alongside this one (see lib_core/libc/mkfile's own darwin
 * story) -- no execution here, only cross-compiled and struct-checked
 * against the Go source.
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
