#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"


/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
const
char*
utfutf(const char *s1, const char *s2)
{
	const char *p;
	long f, n1, n2;
	Rune r;

	n1 = chartorune(&r, s2);
	f = r;
	if(f <= Runesync)		/* represents self */
		return strstr(s1, s2);

	n2 = strlen(s2);
	for(p=s1; (p=utfrune(p, f)) != 0; p+=n1)
		if(strncmp(p, s2, n2) == 0)
			return p;
	return 0;
}
