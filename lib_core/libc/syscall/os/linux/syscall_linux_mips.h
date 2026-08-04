/* Glue the generated zsyscall_linux_mips.c needs: the raw mips
 * trampoline (lib_core/libc/syscall/os/linux/svc_mips.s) and this OS's
 * syscall numbers.
 */
#include "numbers_mips.h"

extern long _syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);

/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * fork/execve/wait4 are ordinary legacy-numbered syscalls here (same
 * shape as 386/arm) and generated straight to their _sys-prefixed raw
 * names by syscall_linux_mips.decl -- no shim needed for them.
 *
 * pipe is the one exception on this arch: numbers_mips.h's own comment
 * explains why o32's real sys_pipe (sysm_pipe) can't go through the
 * generic _syscall6 trampoline at all -- it returns the two fds
 * directly in $v0/$v1, not through the pointer argument every other
 * arch's sys_pipe writes to, and _syscall6 only ever surfaces $v0.
 * _sysrawpipe2 (generated from SYS_pipe2, the normal pointer+flags
 * shape) is the substitute; this shim wraps it with flags=0 so
 * port/pipe.c can call the same _syspipe(fd) name on every arch.
 */
extern int _sysrawpipe2(int *fd, int flags);

int _syspipe(int *fd)
{
	return _sysrawpipe2(fd, 0);
}
