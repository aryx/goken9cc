/* Glue the generated zsyscall_linux_sparc.c needs: the raw sparc
 * trampoline (lib_core/libc/syscall/os/linux/svc_sparc.s) and this
 * OS's syscall numbers.
 */
#include "numbers_sparc.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
