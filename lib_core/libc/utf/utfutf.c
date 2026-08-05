#include <u.h>
#include <libc.h>

/* claude: dropped local "utf.h"/"utfdef.h" includes -- same
 * "macro redefined: nelem"/"nil" fix already applied to utfrune.c/
 * utfecpy.c (see utfrune.c's own header comment): both duplicate
 * macros <libc.h> already brings in, and everything this file uses
 * (Rune, chartorune, Runesync, strstr, utfrune, strncmp, strlen) is
 * already declared there. Found wiring utfutf() into the build for
 * the first time (rc self-hosting's Xbackq -- docs/claude_notes/
 * notes_libc_selfhost.txt's "rc self-hosting survey"). */


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
