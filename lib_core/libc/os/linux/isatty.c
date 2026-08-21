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

/* isatty() -- rc self-hosting's real gap (docs/claude_notes/
 * notes_libc_selfhost.txt's "rc self-hosting survey"): rc/plan9.c's
 * own Isatty() is built on fd2path(), a real Plan9 syscall with no
 * POSIX equivalent at all, so rc/goken.c needs a genuinely new
 * primitive here, not a port of an existing principia/lib9 file.
 *
 * A real ioctl(fd, TCGETS, &termios) probe, the same technique every
 * real libc's isatty(3) uses: it succeeds only on a real terminal fd,
 * fails (ENOTTY) on anything else. The struct termios this writes
 * into is never read -- only the ioctl's own success/failure matters
 * here -- so `buf` is sized generously (128 bytes) rather than exactly
 * (glibc's own struct termios is ~60 bytes, mips's NCCS differs
 * slightly) -- same "too-large zeroed buffer is harmless, too-small is
 * a real stack overflow" asymmetry os/windows/exec.c's own
 * STARTUPINFOA buffer comment already established for a struct this
 * project has no real header for.
 *
 * TCGETS itself is 0x5401 on every arch here except mips (0x540d) --
 * confirmed against real kernel headers (asm-generic/ioctls.h vs
 * mips/asm/ioctls.h), the same "mips' ABI diverges from the generic
 * table" pattern Tier 6's own Ksigaction/SYS_kill/SYS_ioctl numbers
 * already hit repeatedly.
 */
#ifdef mips
#define TCGETS 0x540d
#else
#ifdef power
/* claude: PowerPC uses the BSD/SunOS-style ioctl encoding (like mips),
 * not the generic 0x5401 -- confirmed by actually compiling+running a
 * probe with the real powerpc-linux-gnu-gcc cross compiler under
 * qemu-ppc (`printf("%x", TCGETS)` against
 * <asm/ioctls.h>/<asm/termios.h>), not computed by hand from the
 * encoding macros (arch/powerpc/include/uapi/asm/ioctl.h's own
 * _IOC_SIZEBITS=13/_IOC_DIRBITS=3, non-default, made a manual
 * computation error-prone enough not to trust blindly).
 */
#define TCGETS 0x402c7413
#else
#define TCGETS 0x5401
#endif
#endif

extern int _sysioctl(int fd, ulong request, void *arg);

int
isatty(fdt fd)
{
	byte buf[128];

	return _sysioctl(fd, TCGETS, buf) >= 0;
}
