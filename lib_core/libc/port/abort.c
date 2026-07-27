#include <u.h>
#include <libc.h>

/* No signal/core-dump mechanism wired up yet (see lib_core/libc/syscall/) --
 * just terminate the process. Revisit once this libc has a real signal
 * layer.
 */
void
abort(void)
{
	exit(1);
}
