#include <u.h>
#include <libc.h>

// used? seems dead. Maybe just a hook (should be declared in libc.h though)
void (*_sysfatal)(char*, ...);

void
sysfatal(char *fmt, ...)
{
	char buf[256];
	va_list arg;

	va_start(arg, fmt);
	if(_sysfatal)
		(*_sysfatal)(fmt, arg);
	vseprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);

	__fixargv0();
	fprint(STDERR, "%s: %s\n", argv0 ? argv0 : "<prog>", buf);
	exits("fatal");
}

