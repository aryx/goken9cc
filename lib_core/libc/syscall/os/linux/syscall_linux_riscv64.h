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

/* claude: vlong-returning twin of _syscall6 above -- see
 * syscall_linux_amd64.h's identical comment. svc_riscv64.s's own
 * _syscall6v additionally uses a full 64-bit MOV (not MOVW) for the
 * return-value copy, unlike _syscall6's MOVW.
 */
extern vlong _syscall6v(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* See syscall_linux_arm64.h's identical comment: this arch's "generic"
 * Linux ABI has no legacy 3-arg open(), only openat() -- _sysopen()
 * bridges the gap with AT_FDCWD so os/linux/open.c can call the same
 * name on every arch.
 */
#define AT_FDCWD (-100)

extern long openat(int dirfd, void *path, int flags, int mode);

long _sysopen(void *path, int flags, int mode)
{
	return openat(AT_FDCWD, path, flags, mode);
}
