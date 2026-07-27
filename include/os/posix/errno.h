/* Minimal POSIX-style errno, needed only because fmt/fltfmt.c and
 * fmt/strtod.c #include <errno.h> directly (for ERANGE, when a parsed
 * float overflows). include/errno.h is a thin shim to this file so
 * that literal #include <errno.h> resolves here instead of a real
 * Unix header (7c never searches /usr/include).
 */
#ifndef _OS_POSIX_ERRNO_H_
#define _OS_POSIX_ERRNO_H_ 1

extern int errno;

#define ERANGE 34

// needed by fmt/errfmt.c (the %r format verb)
extern char* strerror(int);

#endif
