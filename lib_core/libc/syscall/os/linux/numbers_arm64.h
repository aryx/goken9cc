/* Linux arm64 syscall numbers (the "generic" Linux syscall ABI, shared
 * with riscv64 -- see asm-generic/unistd.h upstream). Hand-written for
 * now since only a couple are needed; a kernel-header-scraping generator
 * (like GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once
 * more are needed.
 */

// no legacy SYS_open in the "generic" Linux ABI (removed from
// asm-generic/unistd.h -- newer archs are openat()-only, see
// zsyscall_linux_arm64.c's own hand-written _sysopen() shim in
// syscall_linux_arm64.h, which calls this with AT_FDCWD)
/* claude: no legacy SYS_unlink either, for the same reason there's no
 * SYS_open -- the "generic" ABI kept only the *at() forms, so remove()
 * is built on unlinkat(AT_FDCWD, path, 0) by a shim in
 * syscall_linux_arm64.h, exactly parallel to _sysopen()/openat() below.
 * chdir survived unchanged (it takes no dirfd to generalize over).
 * Both numbers read straight off scripts/syscall.tbl upstream ("35
 * common unlinkat", "49 common chdir") -- the single table arm64,
 * riscv and riscv64 all generate from, which is why their three
 * numbers_*.h files agree here too.
 */
/* claude: mkdirat, for Plan9 create()'s DMDIR bit. There is
 * deliberately no rmdir here: this ABI expresses it as
 * unlinkat(..., AT_REMOVEDIR) using the number already below, so
 * remove()'s directory case costs no new syscall on these archs at all
 * -- see syscall_linux_arm64.h's _sysrmdir(). "34 common mkdirat".
 */
/* claude: dup survived into this ABI unchanged (it takes no path to
 * generalize over), but dup2 did not -- dup3(old,new,0) replaces it,
 * and access is faccessat. See syscall_linux_arm64.h for both shims,
 * and numbers_386.h for why Plan9's dup(old,new) needs both forms.
 * "23 common dup", "24 common dup3", "48 common faccessat".
 */
#define SYS_dup	23
#define SYS_dup3	24
#define SYS_mkdirat	34
#define SYS_unlinkat	35
#define SYS_faccessat	48
#define SYS_chdir	49
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
/* claude: brk -- the raw kernel primitive port/sbrk.c is built on (it
 * is sbrk, not brk, that the toolchain's own callers use; see
 * include/os/mem.h and port/sbrk.c). Unlike open/unlink/mkdir/dup2/
 * access above, brk has NO *at()-style replacement and was not dropped
 * from the generic table -- it is the same call under the same name,
 * just renumbered. Note Linux's brk(2) does not use the usual
 * negative-errno convention: it returns the NEW break, and signals
 * failure by returning the UNCHANGED old one; the public Plan9-shaped
 * brk() (0/-1) is the shim in syscall_linux_arm64.h. From
 * include/uapi/asm-generic/unistd.h ("#define __NR_brk 214"), and
 * common to rv32/rv64 (not one of the 32-vs-64 split rows).
 */
#define SYS_brk	214
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
#define SYS_getpid	172
#define SYS_getcwd	17
#define SYS_clock_gettime	113
#define SYS_clock_nanosleep	115
/* claude: the stat family (Tier 3). Confirmed against this host's own
 * /usr/include/asm-generic/unistd.h (this box is aarch64, so the
 * generic ABI table resolves natively here): fstat=80, fchmod=52,
 * ftruncate=46. Same table riscv64 shares below.
 */
#define SYS_fstat	80
#define SYS_fchmod	52
#define SYS_ftruncate	46
/* claude: dirread's other raw call (Tier 3.5) -- openat already exists
 * above for _sysopen's own bridge. Confirmed against this host's own
 * /usr/include/asm-generic/unistd.h: getdents64=61.
 */
#define SYS_getdents64	61

// renameat2(2) -- unconditionally in the generic unistd.h (no
// __ARCH_WANT_* guard, unlike plain renameat), used uniformly across
// all 6 arches instead of legacy rename/renameat -- see
// numbers_amd64.h's own comment.
#define SYS_renameat2	276
/* claude: Tier 4 process control -- see numbers_386.h's fuller comment
 * on the port/*.c normalization these route through. Unlike the
 * legacy-numbered arches, this "generic" ABI dropped plain fork() and
 * pipe() the same way it already dropped open()/unlink()/dup2() above
 * -- confirmed against this host's own /usr/include/asm-generic/
 * unistd.h: no __NR_fork or __NR_pipe at all, only clone=220 and
 * pipe2=59 (both already gated by nothing -- unconditional, unlike the
 * time32-gated calls in numbers_riscv.h). execve=221 and wait4=260
 * did survive under their usual names/shapes. wait4 itself IS gated
 * (`#if defined(__ARCH_WANT_TIME32_SYSCALLS) || __BITS_PER_LONG != 32`)
 * but arm64 is 64-bit (__BITS_PER_LONG==64) so the gate is always open
 * here -- riscv32 is the one arch in this family where it is NOT (see
 * numbers_riscv.h). syscall_linux_arm64.h's own fork()/pipe() shims
 * (over _sysclone(SIGCHLD,...)/pipe2(...,0)) bridge the two dropped
 * calls, exactly like its existing _sysopen()-over-openat() shim above.
 */
#define SYS_clone	220
#define SYS_execve	221
#define SYS_wait4	260
#define SYS_pipe2	59

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story (arm64's own
 * flags/mask widths and 1-word sigset_t are the identical 64-bit
 * story amd64's own comment already covers). kill=129,
 * rt_sigreturn=139, rt_sigaction=134: the "generic" ABI numbers (this
 * arch's own asm-generic/unistd.h, no per-arch override -- same story
 * numbers_riscv64.h's own comment already tells for openat/getdents64
 * above). EMPIRICALLY VERIFIED NATIVELY (this project's own dev host
 * is real arm64 Linux): a probe program built with the host's real
 * gcc registered a handler via exactly this struct shape and these
 * numbers, sent itself SIGALRM via a raw kill(2) syscall, and observed
 * the handler run -- confirming both the numbers and, unlike every
 * other Linux arch here, that this arch's kernel supplies its own
 * default restorer even with NO SA_RESTORER/restorer set at all (a
 * real, positively-confirmed fact, not merely "didn't test it"). This
 * libc's own arch/arm64/sigrestore.s still supplies an explicit one
 * anyway, the portable/standards-compliant choice, not relying on
 * that leniency.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uvlong	flags;
	void	(*restorer)(void);
	uvlong	mask;
};
#define SYS_kill	129
#define SYS_rt_sigaction	134
/* claude: unused by os/linux/notify.c on this arch -- see
 * numbers_arm.h's identical comment (tested WITHOUT SA_RESTORER, the
 * kernel's own default worked fine -- matches the native-host finding
 * a few lines up too). Kept as accurate reference facts. */
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	139

// rc self-hosting's Isatty() (os/linux/isatty.c) -- confirmed against
// real aarch64-linux-musl/bits/syscall.h, matching this arch's own
// SYS_kill=129 above (same reference table as numbers_riscv*.h's).
#define SYS_ioctl	29

// alarm(): setitimer, not the alarm(2) syscall -- the generic syscall
// ABI has no SYS_alarm at all (same reason there is no fork() or
// pipe() here). See os/linux/alarm.c. Same reference table as
// numbers_riscv*.h, matching this arch's own SYS_kill=129 above.
#define SYS_setitimer	103
