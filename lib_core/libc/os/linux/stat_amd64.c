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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/amd64 -- the one
 * GOOS/arch-specific piece of Tier 3 (docs/claude_notes/plan_syscalls.txt);
 * dirstat()/dirwstat() (by path) are portable on top of these (by fd),
 * see port/dirstat.c and port/dirwstat.c.
 *
 * Struct layout is the kernel's `struct stat` for x86-64, field-for-field
 * from GO/pkg/syscall/ztypes_linux_amd64.go's Stat_t -- the same 2010-era
 * Go snapshot this project's own syscall numbers are already sourced
 * from (see numbers_amd64.h). No host <sys/stat.h> is used or needed:
 * this libc never includes host headers.
 *
 * Every field below that is a real 8-byte quantity on amd64 (Nlink,
 * Blksize, the three Sec/Nsec timespec halves -- NOT just the obviously
 * 64-bit ones like Size) is declared vlong/uvlong, never this project's
 * own `long`/`ulong`: every compiler in this tree defines `long` as 4
 * bytes even on 64-bit arches (SZ_LONG in compilers/*c/gc.h -- see
 * syscall_linux_amd64.h's own comment on the exact same trap for
 * syscall *arguments*). Using `long` here for an 8-byte kernel field
 * would silently misalign every field after it, not just truncate one.
 */
typedef struct Kstat Kstat;
struct Kstat {
	uvlong	dev;
	uvlong	ino;
	uvlong	nlink;
	uint	mode;
	uint	uid;
	uint	gid;
	int	__pad0;
	uvlong	rdev;
	vlong	size;
	vlong	blksize;
	vlong	blocks;
	vlong	atime;
	vlong	atime_nsec;
	vlong	mtime;
	vlong	mtime_nsec;
	vlong	ctime;
	vlong	ctime_nsec;
	vlong	__unused[3];
};

/* S_IFMT family -- POSIX-standard bit values, not Linux-specific, so
 * these are the same on every arch this file group targets.
 */
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

/* claude: mode and length only -- mtime-setting (utimensat) is a
 * deliberate gap, not an oversight. See port/dirwstat.c's own comment
 * for the "~field == 0 means unchanged" sentinel convention nulldir()
 * sets up, and why no caller in this tree needs mtime-setting yet.
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
