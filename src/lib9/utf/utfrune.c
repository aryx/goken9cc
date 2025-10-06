#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"

/* const - removed for go code */
char*
utfrune(const char *s, Rune c)
{
	long c1;
	Rune r;
	int n;

	if(c < Runesync)		/* not part of utf sequence */
		return strchr(s, c);

	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return 0;
			if(c1 == c)
				return (char*)s;
			s++;
			continue;
		}
		n = chartorune(&r, s);
		if(r == c)
			return (char*)s;
		s += n;
	}
	return 0;
}
