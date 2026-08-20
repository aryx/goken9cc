/* Glue the generated zsyscall_linux_alpha.c needs: the raw alpha
 * trampoline (lib_core/libc/syscall/os/linux/svc_alpha.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: like every Plan9 C
 * compiler in this tree, zc's `long` is 4 bytes even on this 64-bit
 * arch (compilers/zc/gc.h's SZ_LONG), so a syscall argument that's
 * actually a pointer (e.g. write()'s buf) would get silently
 * truncated passing through a `long`-typed a1 -- see
 * syscall_linux_amd64.h's identical comment for the full story. num
 * stays `long` since a syscall number always fits comfortably in 32
 * bits.
 */
#include "numbers_alpha.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* claude: vlong-returning twin of _syscall6 above (same trampoline,
 * separate symbol -- see svc_alpha.s's own _syscall6v comment: unlike
 * every other arch here, alpha's _syscall6/_syscall6v bodies are
 * byte-for-byte identical, since v0/R0 doubles as both the syscall-
 * number-in and the C-return-value-out register on this ABI). Used
 * only by lseek/brk's decl entries so their 64-bit results aren't
 * truncated to 32 bits the way every other syscall wrapper here still
 * is (write/read counts, fds -- never large enough in practice for
 * that truncation to matter).
 */
extern vlong _syscall6v(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);
