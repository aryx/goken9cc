#include <u.h>
#include <libc.h>

/* claude: replaced with a plain copy of principia's lib_core/libc/
 * port/utfrrune.c (this project keeps it under utf/, matching the
 * Go-era import's own directory split, not principia's port/) --
 * the previous version here used `const`, which this compiler doesn't
 * reliably support (see docs/claude_notes/notes_libc_selfhost.txt's
 * ls.c porting section for the `ord`-after-a-pointer-declaration bug
 * that same const-avoidance discipline was already applied for), and
 * called strrchr() without it existing anywhere in this tree (fixed
 * separately, port/strrchr.c).
 */
char*
utfrrune(char *s, long c)
{
	long c1;
	Rune r;
	char *s1;

	if(c < Runesync)		/* not part of utf sequence */
		return strrchr(s, c);

	s1 = 0;
	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return s1;
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
}
