#include <u.h>
#include <libc.h>

#undef dup

int
p9dup(fdt old, fdt new)
{
	if(new == -1)
		return dup(old);
	return dup2(old, new);
}
