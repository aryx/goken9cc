/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* notify_none.c -- GOOS has neither notify()/noted() nor postnote()
 * (currently windows: Tier 6 never reached it -- lib_core/libc/mkfile's
 * NOTIFYOFILES has no windows branch at all). Both catchnotes() and
 * expunge() are no-ops: mk still runs recipes, it just can't install
 * its own Ctrl-C/hangup handler or signal a specific child by pid+
 * message -- a real, accepted gap, not an oversight. */
#include	"mk.h"

void
catchnotes()
{
	/* no notify()/noted() to build atnotify() on for this GOOS yet */
}

void
expunge(int pid, char *msg)
{
	USED(pid);
	USED(msg);
	/* no postnote() for this GOOS yet */
}
