/* Glue the generated zsyscall_linux_arm.c needs: the raw arm
 * trampoline (lib_core/libc/syscall/os/linux/svc_arm.s) and this OS's
 * syscall numbers.
 */
#include "numbers_arm.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
