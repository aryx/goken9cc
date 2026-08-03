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

/* dirfstat()/dirfwstat() (include/os/stat.h) for windows. Only one
 * file here (unlike os/linux/ or os/darwin/'s per-$cputype split): only
 * amd64 is implemented on this GOOS at all, and unlike a raw kernel
 * struct stat, BY_HANDLE_FILE_INFORMATION's shape does not vary by
 * arch -- see lib_core/libc/mkfile's STATOFILES comment.
 *
 * fd is cast straight to a HANDLE with no _wingetstdhandle-style 0/1/2
 * special-casing (unlike open.c's fdhandle()) -- nothing in this tree
 * dirfstat()s stdin/stdout/stderr, and open.c's own header comment
 * already documents the fd-as-truncated-HANDLE simplification this
 * relies on.
 *
 * Two real gaps, both documented rather than guessed around:
 *  - dirfwstat() implements length only, not mode. Win32 has no simple
 *    by-HANDLE equivalent of fchmod (SetFileAttributes takes a PATH,
 *    which a bare fd/HANDLE does not carry here) -- setting it would
 *    need FileBasicInfo via SetFileInformationByHandle, a bigger struct
 *    this pass didn't add. Same spirit as access()'s approximate
 *    AREAD/AEXEC in open.c.
 *  - mtime-setting is unimplemented everywhere, not just here -- see
 *    os/linux/stat_amd64.c's dirfwstat() comment.
 * Unverified on a real Windows host, like every os/windows/*.c file
 * added without one -- see winio_amd64.s's own stub-level notes.
 */

/* BY_HANDLE_FILE_INFORMATION, Win32's shape -- kept struct-free in the
 * assembly layer (winio_amd64.s's _winfileinfo) the same way every
 * _sysXxx raw syscall wrapper on the Linux/Darwin side is; this is the
 * one file that needs to know it.
 */
typedef struct Byhandleinfo Byhandleinfo;
struct Byhandleinfo {
	uint	attrs;
	uint	ctime_lo, ctime_hi;
	uint	atime_lo, atime_hi;
	uint	mtime_lo, mtime_hi;
	uint	volserial;
	uint	size_hi, size_lo;
	uint	nlink;
	uint	index_hi, index_lo;
};

#define FILE_ATTRIBUTE_READONLY		0x1
#define FILE_ATTRIBUTE_DIRECTORY	0x10

/* same FILETIME epoch/scale shift as os/windows/time.c's filetime()/
 * time() -- see that file's comment for why the constant is spelled
 * out rather than a bare number. Not shared via a header: two small
 * enum constants duplicated once is cheaper than a new include for it.
 */
enum {
	Tick		= 10000000LL,
	Epochdelta	= ((369LL*365LL + 89LL) * 86400LL)
};

static ulong
filetime2sec(uint lo, uint hi)
{
	uvlong ft;

	ft = ((uvlong)hi << 32) | (uvlong)lo;
	if (ft == 0)
		return 0;
	return (ulong)(ft / Tick - (uvlong)Epochdelta);
}

extern int _winfileinfo(void *handle, void *buf);
extern int _winseteof(void *handle);
extern vlong _winseek(void *handle, vlong offset, int whence);

Dir*
dirfstat(fdt fd)
{
	Byhandleinfo bi;
	Dir *d;

	if (!_winfileinfo((void*)(vlong)fd, &bi))
		return nil;
	d = malloc(sizeof(Dir));
	if (d == nil)
		return nil;
	memset(d, 0, sizeof(Dir));
	d->name = d->uid = d->gid = d->muid = "";
	d->qid.path = ((uvlong)bi.index_hi << 32) | (uvlong)bi.index_lo;
	d->mode = 0666;
	if (bi.attrs & FILE_ATTRIBUTE_READONLY)
		d->mode &= ~0222;
	if (bi.attrs & FILE_ATTRIBUTE_DIRECTORY) {
		d->mode |= DMDIR;
		d->qid.type = QTDIR;
	}
	d->atime = filetime2sec(bi.atime_lo, bi.atime_hi);
	d->mtime = filetime2sec(bi.mtime_lo, bi.mtime_hi);
	d->length = ((vlong)bi.size_hi << 32) | (vlong)bi.size_lo;
	return d;
}

/* claude: length only -- see this file's header comment on why mode
 * is not implemented here. SetEndOfFile truncates to wherever the file
 * pointer currently is, not to an arbitrary length, so this seeks
 * there first; FILE_BEGIN is 0, matching port/seek.c's own note that
 * Win32's FILE_BEGIN/CURRENT/END already equal SEEK_SET/CUR/END.
 */
int
dirfwstat(fdt fd, Dir *d)
{
	void *h;

	if (~d->length == 0)
		return 0;
	h = (void*)(vlong)fd;
	if (_winseek(h, d->length, 0) < 0)
		return -1;
	if (!_winseteof(h))
		return -1;
	return 0;
}
