#include "minilibc.h"

int32
findnull(byte *s)
{
	int32 l;

	if(s == nil)
		return 0;
	for(l=0; s[l]!=0; l++)
		;
	return l;
}
