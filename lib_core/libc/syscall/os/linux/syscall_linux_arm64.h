/* Glue the generated zsyscall_linux_arm64.c needs: the raw arm64
 * trampoline (lib_core/libc/syscall/arch/arm64/svc.s) and this OS's
 * syscall numbers.
 */
#include "numbers_arm64.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
