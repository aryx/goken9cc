/* Glue the generated zsyscall_darwin_arm64.c needs: the raw arm64
 * trampoline (lib_core/libc/syscall/os/darwin/svc_arm64.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: see
 * lib_core/libc/syscall/os/linux/syscall_linux_arm64.h's identical
 * comment for the full story -- 7c's `long` is 4 bytes on arm64 even
 * though pointers are 8, so a `long`-typed a1 silently truncated a
 * pointer argument (e.g. write()'s buf). This is in fact where the bug
 * was actually found: real execution of this darwin/arm64 hello_libc
 * build hit garbage in the upper 32 bits of the outgoing argument
 * slot (dyld doesn't leave that region zeroed the way a fresh Linux
 * process stack did by luck), which is what led to fixing
 * linux/arm64's and linux/amd64's identical, previously-unnoticed
 * latent copy of the same bug too. num stays `long` since a syscall
 * number always fits comfortably in 32 bits.
 */
#include "numbers_arm64.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* claude: vlong-returning twin of _syscall6 above -- see
 * lib_core/libc/syscall/os/linux/syscall_linux_amd64.h's identical
 * comment.
 */
extern vlong _syscall6v(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);
