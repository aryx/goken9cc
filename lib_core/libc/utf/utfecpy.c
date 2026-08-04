#include <u.h>
#include <libc.h>

/* claude: dropped the local #include "utf.h"/"utfdef.h" this file
 * originally had (matching rune.c/utflen.c's own standalone-library
 * style, meant to compile without <libc.h> at all) -- wiring this
 * file into UTFOFILES means it's compiled alongside <libc.h> now (see
 * utfrrune.c's earlier, same-shaped fix), and utfdef.h's nelem/nil
 * macros have no header guard, so together with <libc.h>'s own
 * equivalents (core/macros.h) they collided: "macro redefined: nelem"/
 * "nil", a real 6c fatal error (not a warning), not assumed.
 */

char*
utfecpy(char *to, char *e, const char *from)
{
	char *end;

	if(to >= e)
		return to;
	end = memccpy(to, from, '\0', e - to);
	if(end == nil){
		end = e-1;
		while(end>to && (*--end&0xC0)==0x80)
			;
		*end = '\0';
	}else{
		end--;
	}
	return end;
}
