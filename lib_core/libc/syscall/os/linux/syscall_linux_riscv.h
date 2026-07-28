/* Glue the generated zsyscall_linux_riscv.c needs: the raw riscv
 * trampoline (lib_core/libc/syscall/arch/riscv/svc.s) and this OS's
 * syscall numbers.
 */
#include "numbers_riscv.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
