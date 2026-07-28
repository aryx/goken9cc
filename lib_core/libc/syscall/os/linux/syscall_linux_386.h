/* Glue the generated zsyscall_linux_386.c needs: the raw 386
 * trampoline (lib_core/libc/syscall/arch/386/svc.s) and this OS's
 * syscall numbers.
 */
#include "numbers_386.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
