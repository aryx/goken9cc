/* Glue the generated zsyscall_linux_amd64.c needs: the raw amd64
 * trampoline (lib_core/libc/syscall/arch/amd64/svc.s) and this OS's
 * syscall numbers.
 */
#include "numbers_amd64.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
