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

/* isatty() -- darwin side of the same gap os/linux/isatty.c documents
 * (rc self-hosting's real gap: rc/plan9.c's own Isatty() is
 * fd2path()-based, a real Plan9 syscall with no POSIX equivalent).
 * A real ioctl(fd, TIOCGETA, &termios) probe -- TIOCGETA (not Linux's
 * TCGETS: different name, different encoding, same idea). `buf` is
 * sized generously (128 bytes, real struct termios is 72 on LP64
 * Darwin) rather than exactly, same asymmetry as the Linux version's
 * own comment -- the ioctl's SUCCESS/FAILURE is all that matters
 * here, this file never reads a field back out of it.
 *
 * TIOCGETA's numeric value is derived from real Darwin headers, not
 * guessed: sys/ttycom.h's own `#define TIOCGETA _IOR('t', 19, struct
 * termios)`, expanded via sys/ioccom.h's `_IOR(g, n, t) = IOC_OUT |
 * ((sizeof(t) & IOCPARM_MASK) << 16) | (g<<8) | n` (IOC_OUT=
 * 0x40000000). sizeof(struct termios) on LP64 Darwin (both amd64 and
 * arm64 -- sys/termios.h's own tcflag_t/speed_t are `unsigned long`,
 * 8 bytes on both) is 72 (4 tcflag_t @ 8 bytes + cc_t[20] @ 1 byte +
 * 4 bytes padding to realign + 2 speed_t @ 8 bytes = 32+20+4+16).
 * 0x40000000 | ((72&0x1fff)<<16) | ('t'<<8) | 19
 *   = 0x40000000 | 0x00480000 | 0x00007400 | 0x00000013 = 0x40487413.
 * Same value on arm64: same LP64 struct layout, arch-independent BSD
 * ioctl-group/number encoding.
 */
#define TIOCGETA	0x40487413

extern int _sysioctl(int fd, ulong request, void *arg);

int
isatty(fdt fd)
{
	byte buf[128];

	return _sysioctl(fd, TIOCGETA, buf) >= 0;
}
