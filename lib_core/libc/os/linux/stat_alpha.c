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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/alpha -- see
 * os/linux/stat_amd64.c's own header comment for the general shape;
 * this file is a straight port of it, differing only in the Kstat
 * struct below.
 *
 * Struct layout confirmed against a REAL alpha-linux kernel header
 * installed on this host (linux-libc-dev-alpha-cross package):
 * linux-headers-.../arch/alpha/include/uapi/asm/stat.h's `struct stat`
 * -- the OLDER, narrower struct SYS_fstat=91 actually fills (alpha
 * also has a separate fstat64=427 with wider dev_t/ino_t and
 * nanosecond times, not used here, matching this decl's plain
 * SYS_fstat choice).
 *
 * st_dev/st_ino/st_mode/st_nlink/st_uid/st_gid/st_rdev are all
 * `unsigned int` in the kernel header (uint here, 4 bytes, matching);
 * st_size/st_atime/st_mtime/st_ctime are the kernel's own `long`/
 * `unsigned long` -- 8 bytes on this 64-bit arch's own compiler, NOT
 * this project's `long`/`ulong` (always 4 bytes here -- see
 * syscall_linux_alpha.h's identical warning), so vlong/uvlong. The
 * uint __pad0 below is NOT in the kernel header's own field list --
 * inferred: seven uints (28 bytes) leave st_size one 4-byte word short
 * of the 8-byte alignment its own vlong-equivalent kernel type needs,
 * the same "invisible compiler-inserted padding" amd64's own Kstat
 * (__pad0 between gid and rdev there) already has to account for.
 * UNVERIFIED beyond "compiles and links" -- no real alpha hardware or
 * documented struct dump to check this padding inference against, see
 * docs/claude_notes/notes_arch_alpha.txt.
 */
typedef struct Kstat Kstat;
struct Kstat {
	uint	dev;
	uint	ino;
	uint	mode;
	uint	nlink;
	uint	uid;
	uint	gid;
	uint	rdev;
	uint	__pad0;
	vlong	size;
	uvlong	atime;
	uvlong	mtime;
	uvlong	ctime;
	uint	blksize;
	uint	blocks;
	uint	flags;
	uint	gen;
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

/* claude: mode and length only -- see os/linux/stat_amd64.c's
 * identical comment on the "~field == 0 means unchanged" convention
 * and the deliberate mtime-setting gap.
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

/* claude: dirread()/dirreadall() (Tier 3.5) -- see
 * os/linux/stat_amd64.c's own header comment for the full story, this
 * is an unmodified port (the kernel's linux_dirent64 layout is
 * arch-independent).
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
