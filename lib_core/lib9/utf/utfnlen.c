#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"

int
utfnlen(const char *s, long m)
{
	int c;
	long n;
	Rune rune;
	const char *es;

	es = s + m;
	for(n = 0; s < es; n++) {
		c = *(uchar*)s;
		if(c < Runeself){
			if(c == '\0')
				break;
			s++;
			continue;
		}
		if(!fullrune(s, es-s))
			break;
		s += chartorune(&rune, s);
	}
	return n;
}
