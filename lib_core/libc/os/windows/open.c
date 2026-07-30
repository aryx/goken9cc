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
 * NOT wired into lib_core/libc/mkfile (no GOOS=windows entry point
 * exists there yet -- see winio_amd64.s's own header comment) and NOT
 * yet compile-checked on this session's Linux host. Best-effort source
 * per the user's own "I'll refine on my Windows machine" framing.
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
extern long	_winread(void *handle, void *buf, long n);
extern long	_winwrite(void *handle, void *buf, long n);
extern long	_winclose(void *handle);
extern vlong	_winseek(void *handle, vlong offset, int whence);
extern void	*_wingetstdhandle(int std);

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
	uvlong access;

	switch (mode & 3) {
	case OWRITE:
		access = GENERIC_WRITE;
		break;
	case ORDWR:
		access = GENERIC_READ | GENERIC_WRITE;
		break;
	case OEXEC: /* no POSIX/Win32 "exec-only" open mode */
	case OREAD:
	default:
		access = GENERIC_READ;
		break;
	}
	/* OTRUNC/OCEXEC/ORCLOSE/OEXCL: not translated yet -- would need
	 * TRUNCATE_EXISTING's dwCreationDisposition, HANDLE_FLAG_INHERIT,
	 * FILE_FLAG_DELETE_ON_CLOSE, CREATE_NEW respectively. Same kind of
	 * documented gap as os/linux/open.c's ORCLOSE note.
	 */
	return (fdt)(vlong)_winopen(path, access);
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
