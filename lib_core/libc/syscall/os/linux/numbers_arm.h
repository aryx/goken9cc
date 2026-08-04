/* Linux arm (EABI, 32-bit) syscall numbers (the classic/legacy
 * numbering, not the "generic" one arm64/riscv/riscv64 share --
 * confirmed against tests/c/mini2/linux_arm.s's own already-working
 * write=4/exit=1, same numbers as 386's). Hand-written for now since
 * only a couple are needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: same two numbers as 386's (see numbers_386.h's own comment on
 * why create() needs none) -- confirmed against arch/arm/tools/syscall.tbl
 * upstream ("10 common unlink", "12 common chdir"), which really does
 * still match the i386 legacy numbering here.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir -- see numbers_386.h's comment; arch/arm/tools/
 * syscall.tbl agrees with the i386 legacy numbering here too.
 */
#define SYS_mkdir	39
#define SYS_rmdir	40
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From arch/arm/tools/syscall.tbl.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	63
/* claude: brk -- see numbers_386.h's comment (same number here, from
 * arch/arm/tools/syscall.tbl's "45 common brk"), and
 * syscall_linux_arm.h's brk() for the return-convention shim.
 */
#define SYS_brk	45
/* claude: the "small tier" -- getpid, getwd, time/nsec, sleep.
 *
 * The clock_* choice needs explaining, since the obvious calls would be
 * gettimeofday and nanosleep. Those take a struct timeval/timespec whose
 * fields are the kernel's `long` -- 4 bytes on 32-bit arches, 8 on
 * 64-bit -- so a single portable declaration of that struct is
 * impossible, and every consumer would need an arch-dependent layout.
 *
 * The clock_gettime/clock_nanosleep family avoids that completely. On
 * the 32-bit arches we use the *_time64 forms, whose struct
 * __kernel_timespec is {s64 tv_sec; s64 tv_nsec} by definition; on the
 * 64-bit arches plain clock_gettime/clock_nanosleep already have an
 * 8+8 struct timespec. So the SHAPE is two vlongs everywhere, and
 * os/linux/time.c can be one arch-independent file with no #ifdef.
 * They are also y2038-correct, which gettimeofday on 32-bit is not.
 * This is additionally forced on riscv32, which has no gettimeofday or
 * nanosleep at all (see numbers_riscv.h).
 */
#define SYS_getpid	20
#define SYS_getcwd	183
#define SYS_clock_gettime	403
#define SYS_clock_nanosleep	407
/* claude: note the two names above are deliberately the *plain* ones
 * even though the NUMBERS are the kernel's *_time64 variants
 * (clock_gettime64 / clock_nanosleep_time64). That aliasing is the
 * point: "SYS_clock_gettime" here means "the clock_gettime on this arch
 * whose timespec is 8+8", which is 403 on a 32-bit arch and 113/228 on
 * a 64-bit one. Keeping one name lets os/linux/time.c stay a single
 * arch-independent file instead of growing an #ifdef ladder.
 */
/* claude: the stat family (Tier 3) -- same numbers as numbers_386.h,
 * confirmed against arch/arm/tools/syscall.tbl, which marks these
 * "common" (shared between OABI and EABI, and identical to 386's).
 */
#define SYS_fstat64	197
#define SYS_fchmod	94
#define SYS_ftruncate64	194
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against
 * arch/arm/tools/syscall.tbl: openat=322, getdents64=217 -- NOT the
 * same as 386's above (which has its own, independently-numbered 295/220).
 */
#define SYS_openat	322
#define SYS_getdents64	217
// renameat2(2) -- see numbers_amd64.h's own comment for why this,
// not plain rename/renameat, is used uniformly across all 6 arches.
#define SYS_renameat2	382
/* claude: Tier 4 process control -- see numbers_386.h's fuller comment.
 * Confirmed against arch/arm/tools/syscall.tbl, which agrees with i386's
 * legacy numbering here too: "2 common fork sys_fork", "11 common
 * execve sys_execve", "42 common pipe sys_pipe", "114 common wait4
 * sys_wait4".
 */
#define SYS_fork	2
#define SYS_execve	11
#define SYS_wait4	114
#define SYS_pipe	42

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story, numbers_386.h's for
 * the 8-byte/2-word sigset_t story (identical here: kill=37,
 * rt_sigreturn=173, rt_sigaction=174, same numbers as 386 -- this is
 * one of the rows the legacy 32-bit arches happen to agree on, unlike
 * most of the syscall table). EMPIRICALLY VERIFIED, not just read off
 * a header: a probe program built with a real cross gcc and run under
 * qemu-arm actually registered a handler via this struct shape and
 * numbers, sent itself SIGALRM via kill(2), and observed the handler
 * run -- both with and without an explicit SA_RESTORER/restorer
 * (qemu-user accepted either; this libc's own arch/arm/sigrestore.s
 * supplies one regardless, the portable/standards-compliant choice).
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
/* claude: unused by os/linux/notify.c on this arch -- tested WITHOUT
 * SA_RESTORER and the kernel's own default return mechanism worked
 * fine (unlike 386, where omitting it is not just unneeded but
 * REQUIRED -- see numbers_386.h's own comment for that distinction).
 * Kept as accurate reference facts. */
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	173
