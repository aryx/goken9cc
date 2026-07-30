/* Glue the generated zsyscall_darwin_amd64.c needs: the raw amd64
 * trampoline (lib_core/libc/syscall/os/darwin/svc_amd64.s) and this OS's
 * syscall numbers.
 *
 * a1..a6 are deliberately `vlong`, not `long`: see
 * lib_core/libc/syscall/os/linux/syscall_linux_amd64.h's identical
 * comment for the full story -- 6c's `long` is 4 bytes on amd64 even
 * though pointers are 8, so a `long`-typed a1 silently truncated a
 * pointer argument (e.g. write()'s buf). num stays `long` since a
 * syscall number always fits comfortably in 32 bits.
 */
#include "numbers_amd64.h"

extern long _syscall6(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);

/* claude: vlong-returning twin of _syscall6 above -- see
 * lib_core/libc/syscall/os/linux/syscall_linux_amd64.h's identical
 * comment.
 */
extern vlong _syscall6v(long num, vlong a1, vlong a2, vlong a3, vlong a4, vlong a5, vlong a6);
