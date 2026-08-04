//TODO: integrate back linux_x86_TODO.c but need to adapt to
// also work for arm64 linux, not just x86 linux

// This is stubbed out for the moment. Will revisit when the time comes.
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "mach.h"

int
ctlproc(int pid, char *msg)
{
	sysfatal("ctlproc unimplemented in Linux");
	return -1; // claude: unreachable (sysfatal calls exits()), but this
	           // compiler requires an explicit return at the end of
	           // every non-void function regardless.
}

char*
proctextfile(int pid)
{
	sysfatal("proctextfile unimplemented in Linux");
	return nil; // claude: unreachable, see ctlproc()'s own comment.
}

char*
procstatus(int pid)
{
	sysfatal("procstatus unimplemented in Linux");
	return nil; // claude: unreachable, see ctlproc()'s own comment.
}

Map*
attachproc(int pid, Fhdr *fp)
{
	sysfatal("attachproc unimplemented in Linux");
	return nil; // claude: unreachable, see ctlproc()'s own comment.
}

void
detachproc(Map *m)
{
	sysfatal("detachproc unimplemented in Linux");
}

int
procthreadpids(int pid, int *p, int np)
{
	sysfatal("procthreadpids unimplemented in Linux");
	return -1; // claude: unreachable, see ctlproc()'s own comment.
}
