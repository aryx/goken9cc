#include <stdarg.h>
#include <string.h>
//#include "plan9.h"
#include "utf.h"

long
runestrlen(const Rune *s)
{

	return runestrchr(s, 0) - s;
}
