/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* notify_full.c -- GOOS has notify()/noted() (currently just linux --
 * see rc/mkfile's NOTIFYVARIANTOFILES and lib_core/libc/mkfile's
 * NOTIFYOFILES). Bodies copied unchanged from rc/plan9.c: rc's real
 * note-driven interrupt/hangup handling. */
#include	"rc.h"
#include	"fns.h"
#include	"io.h"
#include	"exec.h"
#include	"getflags.h"

extern bool interrupted;
extern int trap[NSIG];

char *syssigname[] = {
	"exit",		/* can't happen */
	"hangup",
	"interrupt",
	"quit",		/* can't happen */
	"alarm",
	"kill",
	"sys: fp: ",
	"term",
	0
};

void
notifyf(void *v, char *s)
{
	int i;

	USED(v);
	for(i = 0;syssigname[i];i++)
		if(strncmp(s, syssigname[i], strlen(syssigname[i]))==0){
			if(strncmp(s, "sys: ", 5)!=0)
				interrupted = true;
			goto Out;
		}
	pfmt(err, "rc: note: %s\n", s);
	noted(NDFLT);
	return;
Out:
	if(strcmp(s, "interrupt")!=0 || trap[i]==0){
		trap[i]++;
		ntrap++;
	}
	if(ntrap>=32){	/* rc is probably in a trap loop */
		pfmt(err, "rc: Too many traps (trap %s), aborting\n", s);
		abort();
	}
	noted(NCONT);
}

void
Trapinit(void)
{
	notify(notifyf);
}
