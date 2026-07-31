/* Glue the generated zsyscall_linux_riscv.c needs: the raw riscv
 * trampoline (lib_core/libc/syscall/os/linux/svc_riscv.s) and this OS's
 * syscall numbers.
 */
#include "numbers_riscv.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);

/* claude: lseek() on this arch is a shim over llseek(), not a directly
 * generated wrapper: syscall 62 is llseek here, not lseek (see
 * numbers_riscv.h for the kernel table rows). llseek splits the 64-bit
 * offset across two argument slots -- high word first -- and returns 0,
 * delivering the resulting offset by writing it through a pointer
 * instead of returning it.
 *
 * The `long` return type is what port/seek.c's own extern expects on
 * every 32-bit arch here (its #ifdef ladder widens only amd64/arm64/
 * riscv64), so the vlong the kernel writes back is truncated on the way
 * out. That is a real, if theoretical, limit -- files past 2GB would
 * report a wrong position even though llseek SEEKS correctly, since the
 * offset argument is now passed at full 64-bit width. Left as-is
 * deliberately: rv32 is no worse off than 386/arm/mips, whose real
 * 32-bit lseek syscall cannot express a large offset at all. Widening
 * it would mean adding a fourth arch to that ladder, which is the
 * opposite of the direction CLAUDE.md asks for.
 */
extern int _sysllseek(int fd, ulong offhi, ulong offlo, vlong *result, int whence);

long lseek(int fd, vlong offset, int whence)
{
	vlong result;

	if (_sysllseek(fd, (ulong)(offset >> 32), (ulong)offset, &result,
			whence) < 0)
		return -1;
	return (long)result;
}

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

/* claude: same story for the unlink/rmdir/mkdir family over the *at()
 * forms -- see syscall_linux_arm64.h's fuller comment, including why
 * _sysrmdir() is free here (AT_REMOVEDIR, not a second syscall).
 */
#define AT_REMOVEDIR 0x200

extern int unlinkat(int dirfd, char *path, int flags);
extern int mkdirat(int dirfd, char *path, int mode);

int _sysunlink(char *path)
{
	return unlinkat(AT_FDCWD, path, 0);
}

int _sysrmdir(char *path)
{
	return unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

int _sysmkdir(char *path, int mode)
{
	return mkdirat(AT_FDCWD, path, mode);
}
