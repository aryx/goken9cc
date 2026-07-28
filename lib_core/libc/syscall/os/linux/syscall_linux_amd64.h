/* Glue the generated zsyscall_linux_amd64.c needs: the raw amd64
 * trampoline (lib_core/libc/syscall/os/linux/svc_amd64.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: like every Plan9 C
 * compiler in this tree, 6c's `long` is 4 bytes even on this 64-bit
 * arch (compilers/6c/gc.h's SZ_LONG), so a syscall argument that's
 * actually a pointer (e.g. write()'s buf) would get silently
 * truncated passing through a `long`-typed a1 -- see
 * syscall_linux_arm64.h's identical comment for the full story (found
 * via real macOS execution of arm64's hello_libc; fixed here too on
 * the strength of the same root cause applying, not from an observed
 * amd64 failure of its own). num stays `long` since a syscall number
 * always fits comfortably in 32 bits.
 */
#include "numbers_amd64.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);
