/* Shim so literal #include <errno.h> (used directly by fmt/fltfmt.c,
 * fmt/strtod.c) resolves to our own Plan9-flavored errno, not a Unix one.
 *
 * Deliberately placed under arch/arm64/, not a top-level include/errno.h:
 * this dir's -I path is only added by mkfiles/arm64/mkfile, not by
 * mkfiles/boot-gcc/mkfile -- so the gcc/BOOT build (BOOT/lib9/fmt is a
 * symlink to lib_core/libc/fmt, sharing these same source files) still
 * finds the real system <errno.h> (needed for EINTR/EBUSY/etc, which
 * this minimal shim doesn't provide) instead of this one.
 */
#ifndef _ERRNO_H_
#define _ERRNO_H_ 1
#include "../../os/posix/errno.h"
#endif
