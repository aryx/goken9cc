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

/* nulldir() prepares a Dir for dirwstat()/dirfwstat(): every field set
 * to all-ones means "leave unchanged", the same sentinel convention
 * os/$GOOS/stat.c's dirfwstat() checks field by field (~d->mode == 0,
 * etc). A caller wanting to change just one field (say, mode) does
 * nulldir(&d) then d.mode = 0644 before calling dirwstat. Portable and
 * OS-independent -- the sentinel convention is this tree's own, not
 * tied to any one GOOS's wire format.
 */
void
nulldir(Dir *d)
{
	memset(d, ~0, sizeof(Dir));
	d->name = d->uid = d->gid = d->muid = "";
}
