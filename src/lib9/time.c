#include <u.h>
#include <sys/time.h>
#include <time.h>
#ifndef _WIN32
#include <sys/resource.h>
#endif
#define NOPLAN9DEFINES
#include <libc.h>

long
p9times(long *t)
{
#ifdef _WIN32
	memset(t, 0, 4*sizeof(long));
#else
	struct rusage ru, cru;

	if(getrusage(0, &ru) < 0 || getrusage(-1, &cru) < 0)
		return -1;

	t[0] = ru.ru_utime.tv_sec*1000 + ru.ru_utime.tv_usec/1000;
	t[1] = ru.ru_stime.tv_sec*1000 + ru.ru_stime.tv_usec/1000;
	t[2] = cru.ru_utime.tv_sec*1000 + cru.ru_utime.tv_usec/1000;
	t[3] = cru.ru_stime.tv_sec*1000 + cru.ru_stime.tv_usec/1000;
#endif

	/* BUG */
	return t[0]+t[1]+t[2]+t[3];
}

double
p9cputime(void)
{
	long t[4];
	double d;

	if(p9times(t) < 0)
		return -1.0;

	d = (double)t[0]+(double)t[1]+(double)t[2]+(double)t[3];
	return d/1000.0;
}
