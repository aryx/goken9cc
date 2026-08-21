/* Linux powerpc (32-bit) syscall numbers -- the classic/legacy
 * numbering, same table 386/arm share (confirmed against this host's
 * installed powerpc-linux-gnu cross headers,
 * /usr/powerpc-linux-gnu/include/asm/unistd_32.h: write=4/exit=1,
 * same numbers as 386's/arm's).
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: same two numbers as 386's/arm's (see numbers_386.h's own
 * comment on why create() needs none) -- confirmed against this
 * host's cross headers.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir -- see numbers_386.h's comment; same numbers as
 * 386/arm here too.
 */
#define SYS_mkdir	39
#define SYS_rmdir	40
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	63
/* claude: brk -- see numbers_386.h's comment, and
 * syscall_linux_power.h's brk() for the return-convention shim.
 */
#define SYS_brk	45
/* claude: the "small tier" -- getpid, getwd, time/nsec, sleep. See
 * numbers_arm.h's own fuller comment on why clock_gettime/
 * clock_nanosleep (the *_time64 forms on a 32-bit arch) are used here
 * instead of gettimeofday/nanosleep.
 */
#define SYS_getpid	20
#define SYS_getcwd	182
#define SYS_clock_gettime	403
#define SYS_clock_nanosleep	407
/* claude: the stat family (Tier 3) -- same numbers as numbers_386.h/
 * numbers_arm.h, confirmed against this host's cross headers.
 */
#define SYS_fstat64	197
#define SYS_fchmod	94
#define SYS_ftruncate64	194
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against this
 * host's cross headers: openat=286, getdents64=202 -- NOT the same as
 * 386's/arm's own (each independently numbered).
 */
#define SYS_openat	286
#define SYS_getdents64	202
// renameat2(2) -- see numbers_amd64.h's own comment for why this,
// not plain rename/renameat, is used uniformly across all archs here.
#define SYS_renameat2	357
/* claude: Tier 4 process control -- see numbers_386.h's fuller
 * comment. Confirmed against this host's cross headers.
 */
#define SYS_fork	2
#define SYS_execve	11
#define SYS_wait4	114
#define SYS_pipe	42

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story. Struct shape
 * confirmed against the real kernel source
 * (arch/powerpc/include/uapi/asm/signal.h's own "struct sigaction"):
 * handler, flags (ulong), restorer, mask (sigset_t, _NSIG_WORDS=2 on
 * a 32-bit arch) -- identical layout to numbers_arm.h's own struct,
 * not independently verified at runtime the way arm's own comment
 * describes (no probe program built+run for this arch yet).
 *
 * No SA_RESTORER here (like arm/arm64/mips/riscv/riscv64, unlike
 * amd64/386): PowerPC's sigentry (arch/power/sigrestore.s) is a
 * trivial tail-branch, not a real ABI bridge -- REGARG=R3 already
 * matches the kernel's own sig-in-r3 signal-handler convention, the
 * same situation numbers_arm.h's own comment describes for arm's
 * R0. Untested beyond that reasoning by analogy; verify with a real
 * probe (kill(2) + notify()) before trusting this axis blindly.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uint	flags;
	void	(*restorer)(void);
	uint	mask[2];
};
#define SYS_kill	37
#define SYS_rt_sigaction	173
#define __NR_rt_sigreturn	172

// rc self-hosting's Isatty() (os/linux/isatty.c) -- confirmed against
// this host's cross headers, matching this arch's own SYS_kill=37
// above (same reference table as numbers_386.h's/numbers_arm.h's).
#define SYS_ioctl	54

// alarm(): setitimer, not the alarm(2) syscall -- see os/linux/alarm.c
// for why. Same legacy table as numbers_386.h/numbers_arm.h, matching
// this arch's own SYS_kill=37 above.
#define SYS_setitimer	104
