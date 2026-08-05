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

/* dirread()/dirreadall() for windows -- rc self-hosting's own
 * Opendir()/Readdir() (rc/goken.c, copied from rc/plan9.c) are
 * dirread()-based, matching os/linux/stat_amd64.c's and
 * os/darwin/dirread.c's own dirreadbuf() shape: read raw directory
 * entries just for each NAME, then build a real Dir per entry via the
 * already-working dirfstat(), rather than hand-parsing kernel-specific
 * stat fields out of the raw entry.
 *
 * Two real differences from both of those, not simplifications:
 *
 * - Win32 has no getdents64-shaped "read raw entries from an open
 *   directory HANDLE" call at all. FindFirstFileA/FindNextFileA
 *   enumerate by PATH PATTERN ("dir\*"), not by handle -- so, same
 *   trick os/darwin/dirread.c's own fcntl(F_GETPATH) uses, this asks
 *   the kernel for the directory handle's own real path first
 *   (GetFinalPathNameByHandleA) and builds a pattern from that.
 * - Because Find*File enumerates one entry at a time with no natural
 *   "read a batch, come back for the next batch" boundary the way
 *   getdents64's own buffer-per-call shape gives Linux/Darwin, this
 *   does not distinguish dirread() (Linux/Darwin: one batch) from
 *   dirreadall() (loop to EOF) at all -- both always enumerate the
 *   WHOLE directory in one call here. This is not a shortcut: rc/
 *   goken.c's own Readdir() (its only caller in this tree) already
 *   calls dirread() exactly once per "current batch exhausted" event
 *   and never calls it again once it returns fewer entries than
 *   requested, so returning everything in the first call is already
 *   what that caller's own loop expects, not a behavior change it
 *   would notice.
 */
#define MAX_PATH		260
#define WIN32_FIND_DATAA_SIZE	600
#define FIND_DATA_FILENAME_OFF	44
#define INVALID_HANDLE_VALUE	((void*)(vlong)-1)

extern void *_winfindfirst(char *pattern, void *finddata);
extern int _winfindnext(void *handle, void *finddata);
extern int _winfindclose(void *handle);
extern ulong _wingetfinalpath(void *handle, char *buf, ulong buflen, ulong flags);

static long
dirreadbuf(fdt fd, Dir **dp)
{
	char dirpath[MAX_PATH];
	char pattern[MAX_PATH+2];
	char path[2*MAX_PATH+2];
	byte finddata[WIN32_FIND_DATAA_SIZE];
	char *name;
	void *h;
	Dir *d, *nd, *tmp;
	long ndir, cap;
	fdt cfd;
	ulong n;

	n = _wingetfinalpath((void*)(vlong)fd, dirpath, sizeof dirpath, 0);
	if(n == 0 || n >= sizeof dirpath){
		*dp = nil;
		return -1;
	}
	snprint(pattern, sizeof pattern, "%s\\*", dirpath);

	d = nil;
	ndir = 0;
	cap = 0;
	h = _winfindfirst(pattern, finddata);
	if(h == INVALID_HANDLE_VALUE){
		*dp = nil;
		return 0;
	}
	for(;;){
		name = (char*)(finddata + FIND_DATA_FILENAME_OFF);
		if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0){
			snprint(path, sizeof path, "%s\\%s", dirpath, name);
			cfd = open(path, OREAD);
			if(cfd >= 0){
				nd = dirfstat(cfd);
				close(cfd);
				if(nd != nil){
					nd->name = strdup(name);
					if(ndir >= cap){
						cap = cap ? cap*2 : 16;
						tmp = realloc(d, cap * sizeof(Dir));
						if(tmp == nil){
							free(nd);
							free(d);
							_winfindclose(h);
							*dp = nil;
							return -1;
						}
						d = tmp;
					}
					d[ndir++] = *nd;
					free(nd);
				}
			}
		}
		if(!_winfindnext(h, finddata))
			break;
	}
	_winfindclose(h);
	*dp = d;
	return ndir;
}

long
dirread(fdt fd, Dir **dp)
{
	return dirreadbuf(fd, dp);
}

long
dirreadall(fdt fd, Dir **dp)
{
	return dirreadbuf(fd, dp);
}
