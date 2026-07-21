// This is stubbed out for the moment. Will revisit when the time comes.
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

int
ctlproc(int pid, char *msg)
{
	sysfatal("ctlproc unimplemented in Cygwin");
}

char*
proctextfile(int pid)
{
	sysfatal("proctextfile unimplemented in Cygwin");
}

char*
procstatus(int pid)
{
	sysfatal("procstatus unimplemented in Cygwin");
}

Map*
attachproc(int pid, Fhdr *fp)
{
	sysfatal("attachproc unimplemented in Cygwin");
}

void
detachproc(Map *m)
{
	sysfatal("detachproc unimplemented in Cygwin");
}

int
procthreadpids(int pid, int *p, int np)
{
	sysfatal("procthreadpids unimplemented in Cygwin");
}
