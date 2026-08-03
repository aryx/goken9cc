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
