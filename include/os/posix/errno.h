/* Minimal POSIX-style errno, needed only because fmt/fltfmt.c uses
 * errno/ERANGE (when a parsed float overflows). Reached only via
 * include/libc.h's own #include "os/posix/errno.h" -- fmt/fltfmt.c and
 * fmt/strtod.c themselves don't (and shouldn't) #include <errno.h>
 * directly, since that's a portable Plan9-style file shared with the
 * gcc/BOOT build, which gets a real errno.h from the host instead (see
 * BOOT/include/u.h).
 */
#ifndef _OS_POSIX_ERRNO_H_
#define _OS_POSIX_ERRNO_H_ 1

extern int errno;

#define ERANGE 34

// needed by fmt/errfmt.c (the %r format verb)
extern char* strerror(int);

#endif
