/* Glue the generated zsyscall_linux_riscv.c needs: the raw riscv
 * trampoline (lib_core/libc/syscall/os/linux/svc_riscv.s) and this OS's
 * syscall numbers.
 */
#include "numbers_riscv.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);

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

/* claude: same story for remove() over unlinkat() -- see
 * syscall_linux_arm64.h's fuller comment.
 */
extern int unlinkat(int dirfd, char *path, int flags);

int remove(char *path)
{
	return unlinkat(AT_FDCWD, path, 0);
}
