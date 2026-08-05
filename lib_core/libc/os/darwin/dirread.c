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

/* dirread()/dirreadall() for darwin -- rc self-hosting's other real gap
 * (docs/claude_notes/notes_libc_selfhost.txt's "rc self-hosting on
 * darwin"): rc/goken.c's Opendir()/Readdir() (copied from rc/plan9.c)
 * are dirread()-based, and unlike dirfstat()/dirwstat() this had no
 * darwin implementation at all yet.
 *
 * Modeled directly on os/linux/stat_amd64.c's own dirreadbuf(): read
 * raw kernel directory entries just to get each NAME, then build a
 * real Dir per entry via the ALREADY-WORKING dirfstat() rather than
 * hand-parsing kernel-specific stat fields out of the raw entry. The
 * one real difference from Linux's version: Linux opens each entry
 * via openat(dirfd, name, ...), but Darwin has no *at() syscalls at
 * all in this tree (see os/darwin/getwd.c's own fcntl(F_GETPATH)
 * comment for the same absence). So this asks the kernel for the
 * directory's own real path once (fcntl(F_GETPATH), the same trick
 * getwd.c already uses) and opens "dirpath/name" directly instead.
 *
 * struct dirent's layout below is the 64-bit-ino_t variant real
 * <sys/dirent.h> pairs with getdirentries64 specifically (confirmed:
 * that header has two dirent shapes gated on __DARWIN_64_BIT_INO_T,
 * and comments this project's own numbers_amd64.h already has on
 * record establish getdirentries64, not the older 32-bit
 * getdirentries, as the syscall to use here) -- d_ino/d_seekoff are
 * real 8-byte fields (not Linux's 8+8+2+1 shape), and readable through
 * a real C struct for the same reason Linux's own comment gives.
 */
typedef struct DarwinDirent DarwinDirent;
struct DarwinDirent {
	uvlong	ino;
	uvlong	seekoff;
	ushort	reclen;
	ushort	namlen;
	uchar	type;
	char	name[1];
};

#define F_GETPATH	50
#define MAXPATHLEN	1024

extern int _sysfcntl(int fd, int cmd, void *arg);
extern long _sysgetdirentries64(int fd, void *buf, ulong bufsize, void *position);

static long
dirreadbuf(fdt fd, Dir **dp, int all)
{
	uchar buf[8192];
	char dirpath[MAXPATHLEN];
	char path[MAXPATHLEN];
	Dir *d, *nd, *tmp;
	long n, off, ndir, cap;
	vlong position;
	fdt cfd;
	DarwinDirent *de;

	if(_sysfcntl(fd, F_GETPATH, dirpath) < 0)
		return -1;

	d = nil;
	ndir = 0;
	cap = 0;
	position = 0;
	for(;;){
		n = _sysgetdirentries64(fd, buf, sizeof buf, &position);
		if(n <= 0)
			break;
		for(off = 0; off < n; off += de->reclen){
			de = (DarwinDirent*)(buf + off);
			if(de->name[0] == '.' && (de->name[1] == 0 ||
			    (de->name[1] == '.' && de->name[2] == 0)))
				continue;
			snprint(path, sizeof path, "%s/%s", dirpath, de->name);
			cfd = open(path, OREAD);
			if(cfd < 0)
				continue;
			nd = dirfstat(cfd);
			close(cfd);
			if(nd == nil)
				continue;
			nd->name = strdup(de->name);
			if(ndir >= cap){
				cap = cap ? cap*2 : 16;
				tmp = realloc(d, cap * sizeof(Dir));
				if(tmp == nil){
					free(nd);
					free(d);
					return -1;
				}
				d = tmp;
			}
			d[ndir++] = *nd;
			free(nd);
		}
		if(!all)
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
