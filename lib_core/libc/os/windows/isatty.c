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

/* isatty() for windows -- rc self-hosting's own Isatty() (rc/goken.c)
 * routes through this, same as os/linux/isatty.c/os/darwin/isatty.c do
 * on their own GOOSes. Windows has no ioctl(TCGETS)/termios concept at
 * all for either of those files' own technique to carry over -- the
 * real Win32 answer is GetConsoleMode: it succeeds only when the
 * handle refers to an actual console, and fails (leaving `mode`
 * untouched) for a redirected file or pipe. This is the standard
 * technique, not invented here -- it's what MSVCRT's own _isatty()
 * does internally.
 */
#define STD_INPUT_HANDLE	(-10)
#define STD_OUTPUT_HANDLE	(-11)
#define STD_ERROR_HANDLE	(-12)

extern void *_wingetstdhandle(int std);
extern int _wingetconsolemode(void *handle, void *mode);

static void*
fdhandle(fdt fd)
{
	switch(fd){
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

int
isatty(fdt fd)
{
	ulong mode;

	mode = 0;
	return _wingetconsolemode(fdhandle(fd), &mode) != 0;
}
