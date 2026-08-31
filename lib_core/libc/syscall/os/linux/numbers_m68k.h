/* Linux m68k syscall numbers -- the classic/legacy numbering, same
 * table as 386/arm/mips (numbers_386.h) almost verbatim: every number
 * below matches 386's own EXCEPT openat and renameat2, which this
 * arch's table renumbered independently when they were added later.
 * Confirmed against a real m68k-linux-any kernel header found on this
 * machine (/media/pad/extradrive1/pad/software-src/dev-compiler/
 * zig-github/lib/libc/include/m68k-linux-any/asm/unistd_32.h -- no
 * m68k-linux-gnu cross-headers package installed here, unlike sparc's
 * own sparc64-linux-gnu one used for numbers_sparc.h), not transcribed
 * from memory or assumed shared with 386's table.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: unlink/chdir -- see numbers_386.h's identical comment for
 * what these back (Plan9's remove()/chdir()). Confirmed 10/12 against
 * the real m68k unistd_32.h -- matches 386/arm/mips's own numbers.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir -- see numbers_386.h's identical comment.
 * Confirmed 39/40 against the real m68k unistd_32.h -- matches
 * 386/arm/mips's own numbers.
 */
#define SYS_mkdir	39
#define SYS_rmdir	40
/* claude: access/dup/dup2 -- see numbers_386.h's identical comment.
 * Confirmed 33/41/63 against the real m68k unistd_32.h -- all three
 * match 386's own numbers.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	63
/* claude: brk -- see numbers_386.h's identical comment for what this
 * backs (port/sbrk.c) and Linux's own brk(2) return-value convention.
 * Confirmed 45 against the real m68k unistd_32.h -- matches 386.
 */
#define SYS_brk	45
/* claude: the "small tier" -- getpid, getwd, time/nsec, sleep. See
 * numbers_386.h's fuller comment for why clock_gettime/
 * clock_nanosleep (not gettimeofday/nanosleep) and why the two names
 * below deliberately alias the kernel's *_time64 variants (403/407,
 * same struct __kernel_timespec {s64,s64} shape every 32-bit arch
 * here uses) rather than the plain (32-bit-time_t) ones. Confirmed
 * 20/183/403/407 against the real m68k unistd_32.h -- all match 386.
 */
#define SYS_getpid	20
#define SYS_getcwd	183
#define SYS_clock_gettime	403
#define SYS_clock_nanosleep	407
/* claude: the stat family (Tier 3, docs/claude_notes/plan_syscalls.txt).
 * *64 forms mandatory on every 32-bit Linux arch here, same reasoning
 * as numbers_386.h's identical comment. Confirmed 197/94/194 against
 * the real m68k unistd_32.h -- all three match 386's own numbers.
 */
#define SYS_fstat64	197
#define SYS_fchmod	94
#define SYS_ftruncate64	194
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against the
 * real m68k unistd_32.h: openat=288 (NOT 295 like 386), getdents64=220
 * (matches 386) -- this arch's table renumbered openat independently
 * when it was added, same as every other arch here.
 */
#define SYS_openat	288
#define SYS_getdents64	220
// renameat2(2) -- see numbers_amd64.h's own comment for why this,
// not plain rename/renameat, is used uniformly across all arches.
// Confirmed 351 against the real m68k unistd_32.h -- NOT 353 like
// 386, renumbered independently same as openat above.
#define SYS_renameat2	351
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * Confirmed against the real m68k unistd_32.h: fork=2, execve=11,
 * wait4=114, pipe=42 -- all four match 386's own numbers exactly.
 * See numbers_386.h's comment for the port/*.c bridge each goes
 * through.
 */
#define SYS_fork	2
#define SYS_execve	11
#define SYS_wait4	114
#define SYS_pipe	42

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story. Confirmed against
 * the real m68k unistd_32.h: kill=37, rt_sigaction=174 -- both match
 * 386's own numbers. Same 32-bit, _NSIG_WORDS=2 (8-byte) sigset_t
 * shape as every other 32-bit non-mips arch here (numbers_386.h/
 * numbers_arm.h) -- NOT a single 4-byte word.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uint	flags;
	void	(*restorer)(void);
	uint	mask[2];
};
#define SYS_kill	37
#define SYS_rt_sigaction	174
/* claude: SA_RESTORER_VAL/__NR_rt_sigreturn -- real, correct facts
 * about this arch's kernel ABI (confirmed 173 against the real m68k
 * unistd_32.h, matching 386's own value), kept as accurate reference
 * facts the same way numbers_386.h's identical pair is (os/linux/
 * notify.c's own `#ifdef amd64` guard is the actual, deliberate gate
 * on whether SA_RESTORER is ever used -- UNVERIFIED on m68k, no
 * signal test has run here yet).
 */
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	173

// rc self-hosting's Isatty() (os/linux/isatty.c). Confirmed 54
// against the real m68k unistd_32.h -- matches 386/arm's own
// SYS_ioctl.
#define SYS_ioctl	54

// alarm(): setitimer, not the alarm(2) syscall -- see os/linux/alarm.c
// for why. Confirmed 104 against the real m68k unistd_32.h -- matches
// 386/arm's own SYS_setitimer.
#define SYS_setitimer	104
