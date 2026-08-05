/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* notify_full.c -- GOOS has both notify()/noted() and postnote()
 * (currently just linux -- see mk/mkfile's NOTIFYVARIANTOFILES and
 * lib_core/libc/mkfile's NOTIFYOFILES). Bodies copied unchanged from
 * mk/Plan9.c: real Ctrl-C/hangup propagation to child processes. */
#include	"mk.h"

extern void killchildren(char *msg);

int
notifyf(void *a, char *msg)
{
	static int nnote;

	USED(a);
	if(++nnote > 100){	/* until andrew fixes his program */
		fprint(STDERR, "mk: too many notes\n");
		notify(0);
		abort();
	}
	if(strcmp(msg, "interrupt")!=0 && strcmp(msg, "hangup")!=0)
		return 0;
	killchildren(msg);
	return -1;
}

void
catchnotes()
{
	atnotify(notifyf, 1);
}

void
expunge(int pid, char *msg)
{
	postnote(PNPROC, pid, msg);
}
