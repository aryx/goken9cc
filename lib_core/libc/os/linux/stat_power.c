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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/power. See
 * os/linux/stat_amd64.c's own header comment for the general shape;
 * this file is a straight port of it, differing only in the Kstat
 * struct below.
 *
 * Struct layout confirmed against the real kernel source on this host
 * (linux-headers package): arch/powerpc/include/uapi/asm/stat.h's own
 * "struct stat64" ("This matches struct stat64 in glibc2.1. Only used
 * for 32 bit.") -- a simpler, single-ino shape than arm's own EABI
 * struct_stat64 (no leading __pad1/__pad0/__st_ino, no trailing 64-bit
 * `ino` -- dev/ino are plain 8-byte fields from the start, and there is
 * exactly one ino field, used directly below for qid.path).
 *
 * The two __padN gaps are NOT explicit fields in the kernel header's
 * own field list -- inferred from natural 8-byte alignment of the two
 * 8-byte fields that follow them (st_size after __pad2, st_blocks
 * after st_blksize), the same "invisible compiler-inserted padding"
 * inference stat_alpha.c's own __pad0 already documents. Written as
 * explicit uchar[] padding (not relying on qc's own struct layout
 * rules to insert it) so the byte offsets are exactly what the kernel
 * actually writes regardless of how this compiler aligns structs.
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
	ushort	__pad2;
	uchar	__pad2b[6];
	vlong	size;
	int	blksize;
	uchar	__pad3[4];
	vlong	blocks;
	int	atime;
	uint	atime_nsec;
	int	mtime;
	uint	mtime_nsec;
	int	ctime;
	uint	ctime_nsec;
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
		if (_sysftruncate64(fd, (ulong)d->length,
		    (ulong)((uvlong)d->length >> 32)) < 0)
			ret = -1;
	}
	return ret;
}

/* claude: dirread()/dirreadall() (Tier 3.5) -- see stat_arm.c's own
 * fuller comment on the general approach (arch-independent past the
 * Kstat struct above). The kernel's linux_dirent64 layout (fixed-width
 * fields, all on naturally aligned offsets already -- 8+8+2+1, no
 * padding needed) is read through a real C struct rather than manual
 * byte-shifting, so the reclen/ino fields come out correctly on this
 * big-endian target too.
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
