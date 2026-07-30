//TODO: integrate back darwin_x86_TODO.c but need to adapt to
// also work for arm64 darwin, not just x86 darwin

// This is stubbed out for the moment. Will revisit when the time comes.
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

int
ctlproc(int pid, char *msg)
{
	sysfatal("ctlproc unimplemented in Darwin");
}

char*
proctextfile(int pid)
{
	sysfatal("proctextfile unimplemented in Darwin");
}

char*
procstatus(int pid)
{
	sysfatal("procstatus unimplemented in Darwin");
}

Map*
attachproc(int pid, Fhdr *fp)
{
	sysfatal("attachproc unimplemented in Darwin");
}

void
detachproc(Map *m)
{
	sysfatal("detachproc unimplemented in Darwin");
}

int
procthreadpids(int pid, int *p, int np)
{
	sysfatal("procthreadpids unimplemented in Darwin");
}
