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
