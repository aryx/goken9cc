#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"

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
