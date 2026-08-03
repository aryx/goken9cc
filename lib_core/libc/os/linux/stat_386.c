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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/386. See
 * stat_amd64.c's header comment for the overall design.
 *
 * Struct layout is glibc's `struct stat64` (what SYS_fstat64 fills),
 * field-for-field from GO/pkg/syscall/ztypes_linux_386.go's Stat_t --
 * the same source numbers_386.h's own comment already points at for
 * this arch. Note the double inode field: __st_ino is a 32-bit
 * (possibly-truncated) legacy inode nothing here reads; the REAL
 * 64-bit inode is the trailing `ino` field, which is what qid.path
 * below uses. Every field here is naturally sized (uint64/int64
 * fields need no vlong-vs-long care the way arm64/riscv64/amd64 do --
 * this is a 32-bit arch, so this project's own 4-byte `long` was never
 * going to collide with a kernel `unsigned long` here; none of THIS
 * struct's fields even use plain long/unsigned long, only the fixed-
 * width int32/int64 forms glibc's stat64 was defined with).
 */
typedef struct Kstat Kstat;
struct Kstat {
	uvlong	dev;
	ushort	__pad1;
	ushort	__pad0;
	uint	__st_ino;
	uint	mode;
	uint	nlink;
	uint	uid;
	uint	gid;
	uvlong	rdev;
	ushort	__pad2;
	ushort	__pad3;
	vlong	size;
	int	blksize;
	vlong	blocks;
	int	atime;
	int	atime_nsec;
	int	mtime;
	int	mtime_nsec;
	int	ctime;
	int	ctime_nsec;
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
