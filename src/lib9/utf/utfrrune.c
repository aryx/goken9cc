#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"

/* const - removed for go code */
char*
utfrrune(const char *s, Rune c)
{
	long c1;
	Rune r;
	const char *s1;

	if(c < Runesync)		/* not part of utf sequence */
		return strrchr(s, c);

	s1 = 0;
	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return (char*)s1;
			if(c1 == c)
				s1 = s;
			s++;
			continue;
		}
		c1 = chartorune(&r, s);
		if(r == c)
			s1 = s;
		s += c1;
	}
	return 0;
}
