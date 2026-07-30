/* Glue the generated zsyscall_linux_arm64.c needs: the raw arm64
 * trampoline (lib_core/libc/syscall/os/linux/svc_arm64.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: like every Plan9 C
 * compiler in this tree, 7c's `long` is 4 bytes even on this 64-bit
 * arch (compilers/7c/gc.h's SZ_LONG), so a syscall argument that's
 * actually a pointer (e.g. write()'s buf) would get silently
 * truncated passing through a `long`-typed a1 -- found via real macOS
 * execution of hello_libc, where garbage landed in the upper 32 bits
 * of the outgoing argument slot dyld doesn't happen to leave zeroed
 * the way a fresh Linux process stack did by luck (this bug always
 * existed here too, just masked). The generated zsyscall_linux_arm64.c
 * casts each argument to match (see lib_core/libc/mkfile's generation
 * rule for this file, and scripts/mksyscall.sh's own comment) -- num
 * stays `long` since a syscall number always fits comfortably in 32
 * bits. See docs/claude_notes/notes_libc_selfhost.txt for the writeup.
 */
#include "numbers_arm64.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* claude: vlong-returning twin of _syscall6 above -- see
 * syscall_linux_amd64.h's identical comment.
 */
extern vlong _syscall6v(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* This arch's "generic" Linux ABI dropped the legacy 3-arg open()
 * syscall (see numbers_arm64.h) -- openat() is all that's left, so
 * _sysopen() (the raw-POSIX-open name every arch's os/linux/open.c
 * calls, see syscall_linux_amd64.decl's comment on that name) is a
 * hand-written one-line bridge here instead of decl-generated. Not
 * `static inline`: this project's Xc compilers don't reliably support
 * that combination (see other lib_core/libc headers, none of which use
 * it), and this is only ever included by the one zsyscall_linux_arm64.c
 * translation unit anyway. AT_FDCWD (-100) is the same on every Linux
 * arch (asm-generic/fcntl.h) and used to mean "path is relative to the
 * process's own cwd" -- i.e. classic open()'s exact behavior.
 */
#define AT_FDCWD (-100)

extern long openat(int dirfd, void *path, int flags, int mode);

long _sysopen(void *path, int flags, int mode)
{
	return openat(AT_FDCWD, path, flags, mode);
}
