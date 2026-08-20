/* Linux alpha syscall numbers.
 *
 * Unlike every other numbers_*.h in this tree (all hand-transcribed
 * from a Go-era snapshot or a syscall_*.tbl -- see numbers_amd64.h's
 * own comment), these are confirmed straight from a REAL alpha-linux
 * kernel header installed on this host (the linux-libc-dev-alpha-cross
 * package): /usr/alpha-linux-gnu/include/asm/unistd_32.h (despite the
 * "_32" in the filename -- that names the syscall-table generation
 * flavor, not this arch's word size; alpha's own asm/unistd.h pulls it
 * in unconditionally, no separate unistd_64.h exists). Alpha inherited
 * OSF/1's syscall numbering (see that header's own "traditionally the
 * names linux-alpha uses" comment), which is why exit=1, fork=2,
 * read=3, write=4 -- unlike every generic-ABI arch here (arm64,
 * riscv64), and even unlike amd64/386, which renumbered everything
 * relative to the classic BSD ordering. Confirmed genuinely native
 * (not needing an *at()-based shim the way riscv64's SYS_open gap
 * does): alpha kept every legacy syscall (open, unlink, mkdir, rmdir,
 * access, dup2, fork, execve, wait4, pipe) it ever had, so this file's
 * syscall SET matches amd64's decl/numbers shape exactly, unlike
 * riscv64/arm64's generic-ABI-only shims.
 *
 * pipe is the one exception to "matches amd64's shape exactly" above,
 * for an unrelated reason: alpha inherited not just OSF/1's syscall
 * *numbering* but also its legacy BSD-style *calling convention* for a
 * few historically odd syscalls -- confirmed via svc_alpha.s's own a3/
 * R19-error-flag fix (the same non-negative-errno convention o32 mips
 * uses, per numbers_mips.h's own comment), and pipe(2) on this same
 * family of ABIs returns its two fds directly in v0/a4(R20) rather
 * than through the pointer argument every generic-ABI arch's sys_pipe
 * writes to -- _syscall6 only ever surfaces v0/R0, so raw SYS_pipe
 * can't go through it at all. SYS_pipe2 (sys_pipe2, normal
 * pointer+flags shape, confirmed at __NR_pipe2=488 in the same
 * installed alpha-linux-gnu header this file's own numbers came from)
 * is the substitute syscall_linux_alpha.h's own pipe() shim calls
 * with flags=0 instead -- same fix, same reason, as numbers_mips.h's
 * own SYS_pipe2.
 */

#define SYS_exit	1
#define SYS_fork	2
#define SYS_read	3
#define SYS_write	4
#define SYS_close	6
#define SYS_unlink	10
#define SYS_chdir	12
#define SYS_brk		17
#define SYS_lseek	19
#define SYS_getpid	20	/* __NR_getxpid; aliased by uapi/asm/unistd.h */
#define SYS_kill	37
#define SYS_dup		41
#define SYS_pipe	42	/* unused directly -- see this file's own pipe comment above; SYS_pipe2 is the real entry point */
#define SYS_pipe2	488
#define SYS_ioctl	54
#define SYS_open	45
#define SYS_execve	59
#define SYS_fstat	91
#define SYS_dup2	90
#define SYS_access	33
#define SYS_mkdir	136
#define SYS_rmdir	137
#define SYS_fchmod	124
#define SYS_ftruncate	130
#define SYS_getdents64	377
#define SYS_rt_sigaction	352
#define SYS_setitimer	362
#define SYS_wait4	365
#define SYS_getcwd	367
#define SYS_clock_gettime	420
#define SYS_clock_nanosleep	422
#define SYS_openat	450
#define SYS_renameat2	510

/* claude: Ksigaction (Tier 6, os/linux/notify.c) -- UNVERIFIED, unlike
 * every other number/struct in this file. Alpha's own uapi/asm/
 * signal.h (checked directly, same host package as the syscall
 * numbers above) confirms this arch has no SA_RESTORER flag at all
 * (absent from its SA_* list, where every other arch here that lacks
 * one -- mips -- also omits it: numbers_mips.h's own comment, "SA_RESTORER
 * support was removed"), so this follows mips's shape: no restorer
 * field. What could NOT be confirmed from installed headers (uapi
 * headers describe the libc-facing struct sigaction, not the kernel-
 * internal struct k_sigaction rt_sigaction(2) actually reads/writes,
 * and no alpha kernel *source* -- only uapi headers -- is installed on
 * this host) is the internal field ORDER (mips's own struct puts flags
 * before handler, unlike x86's handler-first -- genuinely per-arch in
 * the real kernel source, not derivable from uapi headers alone).
 * Modeled on mips's handler-then-flags-then-mask order as the more
 * common shape across archs checked so far, but this is a guess, not a
 * confirmed fact the way every other entry in this file is -- verify
 * against real alpha kernel source (arch/alpha/kernel/signal.c) or
 * real hardware/qemu execution of notify.exe before trusting it beyond
 * "compiles and links". SIGKILL/SIGUSR1/etc. themselves are also
 * alpha-specific (OSF/1-derived numbering, confirmed from the same
 * uapi/asm/signal.h) but os/linux/notify.c only ever passes SIGINT
 * (2, same value here as everywhere) to postnote()'s kill(), so no
 * per-arch signal-number table is needed here the way Ksigaction is.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uvlong	flags;
	uvlong	mask;
};
#define __NR_rt_sigreturn	351
