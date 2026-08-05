/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* notify_none.c -- GOOS has no notify()/noted() yet (currently darwin
 * and windows -- see rc/mkfile's NOTIFYVARIANTOFILES and
 * lib_core/libc/mkfile's NOTIFYOFILES: darwin only has postnote(),
 * which rc never calls; windows has neither). Trapinit() is a no-op:
 * rc still runs recipes, it just can't install its own note-driven
 * interrupt/hangup handler -- a real, accepted gap, not an oversight,
 * matching mk/notify_partial.c's/notify_none.c's own catchnotes(). */
#include	"rc.h"
#include	"fns.h"
#include	"io.h"
#include	"exec.h"
#include	"getflags.h"

void
Trapinit(void)
{
	/* no notify()/noted() to build this on for this GOOS yet */
}
