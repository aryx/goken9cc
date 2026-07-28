/* Minimal APE (`ANSI/POSIX Environment`) shim: lets UNIX-style C source
 * (`#include <stdio.h>`) build against goken's own libc.a, the way
 * Plan 9's real APE (`/sys/include/ape/stdio.h`) sits in front of its
 * native libc. Only what benchs/ actually uses (printf) is mapped onto
 * the Plan9-native equivalent from libc.h (print) -- extend as more
 * benchmarks need more of stdio.h.
 */
#include <u.h>
#include <libc.h>

#define printf print
