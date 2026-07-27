#include <u.h>
#include <libc.h>

/* storage for the extern declared in include/os/posix/errno.h (via
 * include/arch/arm64/errno.h's shim) */
int errno;
