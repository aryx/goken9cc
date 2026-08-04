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

/* dirmodefmt() (include/os/dir.h), ported near-verbatim from
 * principia's lib_core/libc/9sys/dirmodefmt.c -- a %M fmtinstall()
 * handler turning a Dir.mode into "drwxrwxrwx". Unlike principia's
 * version this does NOT #include <fcall.h> for DMDIR/DMAPPEND/DMAUTH/
 * DMEXCL: this tree already defines all of Dir's mode bits in
 * include/os/dir.h (already pulled in via libc.h), no separate fcall.h
 * exists here at all.
 *
 * Found missing while retrying utilities/files/ls.c after the ctime()
 * work: ls.c's format() calls Bprint(... "%M %C ...", db->mode, ...)
 * but never installs the %M verb (its own `//XXX: fmtinstall('M',
 * dirmodefmt);` was commented out, presumably because dirmodefmt()
 * didn't exist yet) -- an unrecognized verb doesn't consume its
 * vararg, so every argument after it (including %q's char* pointers)
 * read one slot stale, and ls -l segfaulted on the resulting garbage
 * pointer. Confirmed by symptom (a real qemu run of ls -l crashing)
 * before writing this, not assumed from reading format() alone.
 */

static char *modes[] =
{
	"---",
	"--x",
	"-w-",
	"-wx",
	"r--",
	"r-x",
	"rw-",
	"rwx",
};

static void
rwx(long m, char *s)
{
	strncpy(s, modes[m], 3);
}

int
dirmodefmt(Fmt *f)
{
	static char buf[16];
	ulong m;

	m = va_arg(f->args, ulong);

	if(m & DMDIR)
		buf[0]='d';
	else if(m & DMAPPEND)
		buf[0]='a';
	else if(m & DMAUTH)
		buf[0]='A';
	else
		buf[0]='-';
	if(m & DMEXCL)
		buf[1]='l';
	else
		buf[1]='-';
	rwx((m>>6)&7, buf+2);
	rwx((m>>3)&7, buf+5);
	rwx((m>>0)&7, buf+8);
	buf[11] = 0;
	return fmtstrcpy(f, buf);
}
