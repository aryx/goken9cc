#include <u.h>
#include <libc.h>

/* claude: replaced with a plain copy of principia's lib_core/libc/
 * port/utfrune.c -- same fix, same reason, as utfrrune.c's earlier
 * one: the previous version used `const`/`Rune` (Go-era style, this
 * compiler's const support isn't trustworthy -- see notes_libc_
 * selfhost.txt's `ord`-after-a-pointer 6c bug) and its own local
 * `#include "utf.h"`/`"utfdef.h"` duplicates macros <libc.h> already
 * brings in once compiled alongside it (the same "macro redefined:
 * nelem"/"nil" error utfecpy.c hit). Found blocking utilities/misc/
 * unicode.c.
 */
char*
utfrune(char *s, long c)
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
				return nil;
			if(c1 == c)
				return s;
			s++;
			continue;
		}
		n = chartorune(&r, s);
		if(r == c)
			return s;
		s += n;
	}
}
