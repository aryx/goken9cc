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

/* dirfstat()/dirfwstat() (include/os/stat.h) for linux/mips (o32). See
 * stat_amd64.c's header comment for the overall design.
 *
 * Struct layout is the kernel's own struct stat64
 * (arch/mips/include/uapi/asm/stat.h upstream), fetched directly rather
 * than transcribed from memory -- unlike 386/arm/amd64/arm64/riscv64,
 * this project vendors no Go ztypes_linux_mips.go to cross-check
 * against (GO/pkg/syscall/ only has 386/amd64/arm from its 2010
 * snapshot). o32 is a 32-bit ABI, so unlike arm64/riscv64/amd64 there
 * is no long-vs-vlong trap: every `unsigned long`/`long` field here
 * really is 4 bytes, matching this project's own ulong/long exactly.
 * Only st_ino/st_size/st_blocks (`long long`) need vlong/uvlong.
 *
 * dirfwstat() below is mode-only on this arch -- see numbers_mips.h's
 * SYS_ftruncate64 comment for why length-truncation is a deliberate
 * gap here (o32's calling convention register-pads 64-bit syscall
 * arguments, unlike 386/arm/amd64/arm64/riscv64), on top of
 * stat_amd64.c's dirfwstat() comment on the mtime gap every arch shares.
 */
typedef struct Kstat Kstat;
struct Kstat {
	uint	dev;
	uint	pad0[3];
	uvlong	ino;
	uint	mode;
	uint	nlink;
	uint	uid;
	uint	gid;
	uint	rdev;
	uint	pad1[3];
	vlong	size;
	int	atime;
	uint	atime_nsec;
	int	mtime;
	uint	mtime_nsec;
	int	ctime;
	uint	ctime_nsec;
	uint	blksize;
	uint	pad2;
	vlong	blocks;
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

int
dirfwstat(fdt fd, Dir *d)
{
	int ret;

	ret = 0;
	if (~d->mode != 0) {
		if (_sysfchmod(fd, (int)(d->mode & 0777)) < 0)
			ret = -1;
	}
	return ret;
}
