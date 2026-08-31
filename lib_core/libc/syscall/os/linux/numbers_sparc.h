/* Linux sparc (32-bit, sparc32/sparc-linux) syscall numbers -- its own
 * distinct table, NOT the "legacy i386/arm" numbering some of these
 * happen to coincide with (write=4/exit=1/open=5/close=6/unlink=10/
 * chdir=12/access=33/kill=37/dup=41/pipe=42/mkdir=136/rmdir=137 all
 * match 386's own numbers_386.h by coincidence; brk/execve/wait4/
 * rt_sigaction/rt_sigreturn do NOT -- see this file's own SYS_brk/
 * SYS_execve/SYS_wait4/SYS_rt_sigaction comments below). Confirmed
 * against this host's installed sparc64-linux-gnu cross headers,
 * /usr/sparc64-linux-gnu/include/asm/unistd_32.h, the same reference
 * already used for tests/s/exit/exit_linux_sparc.s's own write=4/
 * exit=1 (see that file's comment) -- not transcribed from memory or
 * assumed-shared with another arch's table.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: unlink/chdir -- see numbers_386.h's identical comment for
 * what these back (Plan9's remove()/chdir()). Confirmed 10/12 against
 * unistd_32.h; matches 386/arm/mips's own numbers, unlike several
 * others below.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir -- see numbers_386.h's identical comment.
 * Confirmed 136/137 against unistd_32.h -- NOT 39/40 like 386/arm/
 * mips, this arch's table renumbered both independently.
 */
#define SYS_mkdir	136
#define SYS_rmdir	137
/* claude: access/dup/dup2 -- see numbers_386.h's identical comment.
 * access=33 and dup=41 happen to match 386's own numbers; dup2=90
 * does NOT (386 has 63).
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	90
/* claude: brk -- see numbers_386.h's identical comment for what this
 * backs (port/sbrk.c) and Linux's own brk(2) return-value convention.
 * Confirmed 17 against unistd_32.h -- NOT 45 like 386/arm.
 */
#define SYS_brk	17
/* claude: the "small tier" -- getpid, getwd, time/nsec, sleep. See
 * numbers_386.h's fuller comment for why clock_gettime/
 * clock_nanosleep (not gettimeofday/nanosleep) and why the two names
 * below deliberately alias the kernel's *_time64 variants (403/407,
 * same struct __kernel_timespec {s64,s64} shape every 32-bit arch
 * here uses) rather than the plain (32-bit-time_t) ones.
 */
#define SYS_getpid	20
#define SYS_getcwd	119
#define SYS_clock_gettime	403
#define SYS_clock_nanosleep	407
/* claude: the stat family (Tier 3, docs/claude_notes/plan_syscalls.txt).
 * *64 forms mandatory on every 32-bit Linux arch here, same reasoning
 * as numbers_386.h's identical comment. Confirmed against unistd_32.h:
 * fstat64=63 (NOT 197 like 386), fchmod=124 (NOT 94),
 * ftruncate64=84 (NOT 194).
 */
#define SYS_fstat64	63
#define SYS_fchmod	124
#define SYS_ftruncate64	84
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against
 * unistd_32.h: openat=284, getdents64=154 -- neither matches 386's
 * own (295/220), this arch's table renumbered both independently as
 * usual.
 */
#define SYS_openat	284
#define SYS_getdents64	154
// renameat2(2) -- see numbers_amd64.h's own comment for why this,
// not plain rename/renameat, is used uniformly across all arches.
// Confirmed 345 against unistd_32.h.
#define SYS_renameat2	345
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * Confirmed against unistd_32.h -- fork=2 matches 386, but
 * execve=59 and wait4=7 do NOT (386 has 11/114); pipe=42 matches.
 * See numbers_386.h's comment for the port/*.c bridge each goes
 * through.
 */
#define SYS_fork	2
#define SYS_execve	59
#define SYS_wait4	7
#define SYS_pipe	42

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story. Confirmed against
 * unistd_32.h: kill=37 (matches 386), rt_sigaction=102 (386 has 174 --
 * does NOT match). Same 32-bit, _NSIG_WORDS=2 (8-byte) sigset_t shape
 * as every other 32-bit non-mips arch here (numbers_386.h/numbers_arm.h) --
 * NOT a single 4-byte word.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uint	flags;
	void	(*restorer)(void);
	uint	mask[2];
};
#define SYS_kill	37
#define SYS_rt_sigaction	102
/* claude: SA_RESTORER_VAL/__NR_rt_sigreturn -- real, correct facts
 * about this arch's kernel ABI (confirmed 101 against unistd_32.h),
 * kept as accurate reference facts the same way numbers_386.h's
 * identical pair is (os/linux/notify.c's own `#ifdef amd64` guard is
 * the actual, deliberate gate on whether SA_RESTORER is ever used --
 * UNVERIFIED on sparc, no signal test has run here yet).
 */
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	101

// rc self-hosting's Isatty() (os/linux/isatty.c). Confirmed 54
// against unistd_32.h -- matches 386/arm's own SYS_ioctl.
#define SYS_ioctl	54

// alarm(): setitimer, not the alarm(2) syscall -- see os/linux/alarm.c
// for why. Confirmed 83 against unistd_32.h -- NOT 104 like 386/arm.
#define SYS_setitimer	83
