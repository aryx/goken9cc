/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* notify_partial.c -- GOOS has postnote() but not notify()/noted()
 * (currently darwin: Tier 6 stopped at postnote()-only there,
 * XNU's sigtramp requirement being a genuine, still-unresolved
 * design question -- see docs/claude_notes/notes_libc_api_design.txt
 * and notes_libc_selfhost.txt's darwin section). expunge() (kill a
 * child by pid+message) still works; catchnotes() (install mk's own
 * Ctrl-C/hangup handler via atnotify(), which itself calls notify())
 * would fail to link, so it's a no-op here instead -- mk still runs
 * recipes and propagates a real Ctrl-C via the kernel's own default
 * SIGINT handling (which kills mk and, on most shells, its
 * still-running child too), it just doesn't get mk's own
 * kill-the-rest-of-the-job-then-exit-cleanly behavor for it. */
#include	"mk.h"

void
catchnotes()
{
	/* no notify()/noted() to build atnotify() on for this GOOS yet */
}

void
expunge(int pid, char *msg)
{
	postnote(PNPROC, pid, msg);
}
