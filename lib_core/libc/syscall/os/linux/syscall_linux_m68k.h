/* Glue the generated zsyscall_linux_m68k.c needs: the raw m68k
 * trampoline (lib_core/libc/syscall/os/linux/svc_m68k.s) and this
 * OS's syscall numbers.
 */
#include "numbers_m68k.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
