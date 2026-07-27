#include <u.h>
#include <libc.h>

/* minimal: no real per-errno message table yet (this libc doesn't
 * populate errno with real per-syscall codes yet either -- see
 * lib_core/libc/syscall/) */
char*
strerror(int e)
{
	if(e == 0)
		return "no error";
	return "error";
}
