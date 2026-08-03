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

/* claude: dirread()/dirreadall() (Tier 3.5). getdents64 gives names
 * (plus a cheap type hint this code doesn't use, since dirfstat()
 * above already gives the real mode/size/times), so each name is
 * turned into a full Dir by opening it relative to the directory fd
 * -- openat(), never a concatenated path string, since dirread's own
 * Plan9 API (include/os/dir.h) only ever hands this an fd, no path --
 * and calling this file's own dirfstat() above. "." and ".." are
 * skipped, matching every readdir()-based implementation (Plan9
 * directories never list them at all). One dirread() call returns
 * whatever fit in one getdents64 buffer -- not an artificial fixed
 * count -- which is at least as close to Plan9's own "however much
 * fit in one read()" semantics as BOOT/lib9's host-readdir()-capped-
 * at-10 approach. dirreadall() loops until getdents64 reports EOF (0).
 *
 * The kernel's linux_dirent64 layout (fixed-width fields, all on
 * naturally aligned offsets already -- 8+8+2+1, no padding needed) is
 * read through a real C struct rather than manual byte-shifting, so
 * the reclen/ino fields come out correctly on mips's big-endian target
 * too, not just the little-endian arches.
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
