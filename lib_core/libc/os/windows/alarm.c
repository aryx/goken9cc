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

/* alarm() for windows -- a real, documented -1 always, same shape and
 * same reasoning as os/windows/fork.c's.
 *
 * The missing piece is not the timer. Win32 has several (SetTimer,
 * CreateWaitableTimer, timeSetEvent, a thread plus Sleep), and any of
 * them could count down milliseconds perfectly well. What is missing is
 * anywhere to DELIVER the expiry to: alarm()'s entire contract is
 * "post an `alarm` note to this process", and this GOOS has no note
 * mechanism at all -- lib_core/libc/mkfile's NOTIFYOFILES gives windows
 * neither notify()/noted() nor even postnote(), and mk/mkfile and
 * rc/mkfile both select their notify_none.c variants here for the same
 * reason. An armed timer would therefore have nothing to do on expiry
 * except terminate the process, which is emphatically not what a
 * caller asking for an alarm wants.
 *
 * So this is a genuine, permanent-until-notes-exist gap, of exactly
 * the kind rc/rfork_windows.c already documents: implementing it
 * "successfully" would be worse than failing, because a caller that
 * checks the return value can fall back, while one that trusts a
 * silent success gets a timeout that never fires. If windows ever
 * grows a real notify()/noted() (which would need a fair amount more
 * than this: a delivery thread, a suspend/resume dance around the main
 * thread, and a Notes-vs-SEH story), this file becomes a small
 * CreateWaitableTimer wrapper over that.
 *
 * Kept at the libc level rather than caller-local for the same reason
 * fork() is: nothing in this tree's own windows/ files calls alarm()
 * internally, so this cannot change any existing behaviour, only turn
 * an "undefined: alarm" link error into a clean runtime failure.
 */
long
alarm(ulong milli)
{
	USED(milli);
	return -1;
}
