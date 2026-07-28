/* Glue the generated zsyscall_linux_mips.c needs: the raw mips
 * trampoline (lib_core/libc/syscall/os/linux/svc_mips.s) and this OS's
 * syscall numbers.
 */
#include "numbers_mips.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
