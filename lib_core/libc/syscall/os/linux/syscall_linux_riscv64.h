/* Glue the generated zsyscall_linux_riscv64.c needs: the raw riscv64
 * trampoline (lib_core/libc/syscall/os/linux/svc_riscv64.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: like every Plan9 C
 * compiler in this tree, jc's `long` is 4 bytes even on this 64-bit
 * arch (see include/arch/riscv64/u.h's header comment), so a syscall
 * argument that's actually a pointer (e.g. write()'s buf) would get
 * silently truncated passing through a `long`-typed a1. The generated
 * zsyscall_linux_riscv64.c casts each argument to match (see
 * lib_core/libc/mkfile's SYSCALLARG=vlong on this file's generation
 * rule, and scripts/mksyscall.sh's own comment) -- num stays `long`
 * since a syscall number always fits comfortably in 32 bits.
 */
#include "numbers_riscv64.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);
