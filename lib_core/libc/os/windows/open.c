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

/* Windows' Plan9-API glue -- open()/read()/write()/close()/seek(),
 * include/os/file.h's shapes, on top of winio_amd64.s's raw kernel32
 * stubs. Unlike Linux/Darwin, there's no separate syscall/ "raw
 * adapter" layer for Windows at all: kernel32.dll IS the one stable
 * ABI applications are meant to call (there's no documented raw
 * NT-syscall-number convention the way Linux/BSD expose one), so the
 * kernel32-calling stubs themselves already live under this os/
 * glue directory instead of a syscall/os/windows/ that would otherwise
 * just be a thin, pointless pass-through.
 *
 * Wired via 'GOOS=windows OSEXTRAFILES=os/windows/winio_$cputype.$O
 * SYSCALLOFILES=' on lib_core/libc/mkfile's command line (see that
 * mkfile's OSFILES/SYSCALLOFILES comments) and verified with a real
 * native build+run on a Windows/Cygwin host -- see
 * tests/c/hello_libc/mkfile's test_windows target and
 * docs/claude_notes/notes_os_windows.txt.
 *
 * The bigger known simplification: fdt is `int` (include/os/file.h)
 * but a real Win32 HANDLE is an 8-byte pointer -- open()'s return value
 * truncates it, and read()/write()/close()/seek() widen it back
 * (sign-extending through vlong). Fine for the small handle values
 * CreateFileA happens to return in practice, not guaranteed by the
 * Win32 API contract. A real fix would keep a small fd-to-HANDLE table
 * instead; not attempted here, flagged as a known gap rather than
 * silently assumed correct.
 */
#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000

#define STD_INPUT_HANDLE	(-10)
#define STD_OUTPUT_HANDLE	(-11)
#define STD_ERROR_HANDLE	(-12)

extern void	*_winopen(char *path, uvlong access);
extern void	*_wincreate(char *path, uvlong access);
extern long	_windelete(char *path);
extern long	_winchdir(char *path);
extern long	_winread(void *handle, void *buf, long n);
extern long	_winwrite(void *handle, void *buf, long n);
extern long	_winclose(void *handle);
extern vlong	_winseek(void *handle, vlong offset, int whence);
extern void	*_wingetstdhandle(int std);

/* claude: the Plan9-mode-bits -> Win32 dwDesiredAccess translation
 * shared by open() and create(), split out for the same reason
 * os/linux/open.c's openflags() was.
 */
static uvlong
winaccess(int mode)
{
	switch (mode & 3) {
	case OWRITE:
		return GENERIC_WRITE;
	case ORDWR:
		return GENERIC_READ | GENERIC_WRITE;
	case OEXEC: /* no POSIX/Win32 "exec-only" open mode */
	case OREAD:
	default:
		return GENERIC_READ;
	}
}

static void*
fdhandle(fdt fd)
{
	switch (fd) {
	case 0:
		return _wingetstdhandle(STD_INPUT_HANDLE);
	case 1:
		return _wingetstdhandle(STD_OUTPUT_HANDLE);
	case 2:
		return _wingetstdhandle(STD_ERROR_HANDLE);
	default:
		return (void*)(vlong)fd;
	}
}

fdt
open(char *path, int mode)
{
	/* OTRUNC/OCEXEC/ORCLOSE/OEXCL: not translated yet -- would need
	 * TRUNCATE_EXISTING's dwCreationDisposition, HANDLE_FLAG_INHERIT,
	 * FILE_FLAG_DELETE_ON_CLOSE, CREATE_NEW respectively. Same kind of
	 * documented gap as os/linux/open.c's ORCLOSE note.
	 */
	return (fdt)(vlong)_winopen(path, winaccess(mode));
}

/* claude: Plan9's create() (include/os/dir.h). On the Unix targets this
 * is open() with two extra flag bits; here it's the same CreateFileA
 * call as open() with a different dwCreationDisposition (CREATE_ALWAYS
 * instead of OPEN_EXISTING) -- see winio_amd64.s's _wincreate.
 *
 * Two gaps, both wider than the Unix side's: `perm` is ignored entirely
 * (Win32 ACLs aren't Unix mode bits, and CreateFileA has no parameter
 * that would carry rwxrwxrwx), and DMDIR is rejected the same way
 * os/linux/open.c rejects it, though here the missing piece is
 * CreateDirectoryA rather than a syscall number. Unverified on a real
 * Windows host.
 */
int
create(char *path, int mode, ulong perm)
{
	if (perm & DMDIR)
		return -1;
	return (int)(vlong)_wincreate(path, winaccess(mode));
}

/* claude: remove()/chdir(). Both Win32 calls return a BOOL (nonzero on
 * success), not POSIX's 0/-1, so unlike every other GOOS in this tree
 * -- where these two are the raw syscall, needing no glue at all --
 * they need the result inverted here. Unverified on a real Windows host.
 */
int
remove(char *path)
{
	return _windelete(path) ? 0 : -1;
}

int
chdir(char *path)
{
	return _winchdir(path) ? 0 : -1;
}

long
read(fdt fd, void *buf, long n)
{
	return _winread(fdhandle(fd), buf, n);
}

long
write(fdt fd, void *buf, long n)
{
	return _winwrite(fdhandle(fd), buf, n);
}

int
close(fdt fd)
{
	return (int)_winclose(fdhandle(fd));
}

vlong
seek(fdt fd, vlong offset, int whence)
{
	return _winseek(fdhandle(fd), offset, whence);
}
