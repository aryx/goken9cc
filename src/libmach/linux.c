//TODO: integrate back linux_x86_TODO.c but need to adapt to
// also work for arm64 linux, not just x86 linux

// This is stubbed out for the moment. Will revisit when the time comes.
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

int
ctlproc(int pid, char *msg)
{
	sysfatal("ctlproc unimplemented in Linux");
}

char*
proctextfile(int pid)
{
	sysfatal("proctextfile unimplemented in Linux");
}

char*
procstatus(int pid)
{
	sysfatal("procstatus unimplemented in Linux");
}

Map*
attachproc(int pid, Fhdr *fp)
{
	sysfatal("attachproc unimplemented in Linux");
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
}
