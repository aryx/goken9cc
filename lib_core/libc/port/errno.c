#include <u.h>
#include <libc.h>

/* storage for the extern declared in include/os/posix/errno.h,
 * reached via include/libc.h's #include "os/posix/errno.h" */
int errno;
