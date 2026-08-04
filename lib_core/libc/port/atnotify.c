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

/* atnotify()/noted-chain (include/os/plan9/note.h) -- Tier 6 notification
 * (docs/claude_notes/plan_syscalls.txt, docs/claude_notes/
 * notes_libc_api_design.txt's "Notes vs. signals" section). Ported as-is
 * from principia's lib_core/libc/port/atnotify.c (itself the same shape
 * as plan9port's BOOT/lib9/atnotify.c -- confirmed identical design
 * across both real references, not just plausible).
 *
 * This is Plan9's OWN higher-level convenience layer on top of the raw
 * notify() syscall, not kernel magic: a fixed-size table of registered
 * handlers, tried in order on every note; the first one that returns
 * true "handled" it (noted(NCONT), resume); if none does, noted(NDFLT)
 * (terminate). Because it is pure portable C built only on notify()/
 * noted() (both already declared in include/os/plan9/note.h for every
 * GOOS), this file is shared UNCHANGED across every GOOS -- the design
 * this project settled on treats notify() itself as the one thing that
 * may differ per GOOS (Plan9: a real syscall, see syscall/os/plan9/
 * svc_$cputype.s; Linux/darwin: not yet implemented, needs sigaction()-
 * based glue first, see the design doc above), while this dispatch
 * layer never needs to know which.
 */

#define	NFN	33
static	int	(*onnot[NFN])(void*, char*);
static	Lock	onnotlock;

static
void
notifier(void *v, char *s)
{
	int i;

	for(i=0; i<NFN; i++)
		if(onnot[i] && ((*onnot[i])(v, s))){
			noted(NCONT);
			return;
		}
	noted(NDFLT);
}

int
atnotify(int (*f)(void*, char*), int in)
{
	int i, n, ret;
	static bool init;

	if(!init){
		notify(notifier);
		init = true;		/* assign = */
	}
	ret = 0;
	lock(&onnotlock);
	if(in){
		for(i=0; i<NFN; i++)
			if(onnot[i] == 0) {
				onnot[i] = f;
				ret = 1;
				break;
			}
	}else{
		n = 0;
		for(i=0; i<NFN; i++)
			if(onnot[i]){
				if(ret==0 && onnot[i]==f){
					onnot[i] = nil;
					ret = 1;
				}else
					n++;
			}
		if(n == 0){
			init = false;
			notify(0);
		}
	}
	unlock(&onnotlock);
	return ret;
}
