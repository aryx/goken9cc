/* Linux riscv64 (rv64) syscall numbers -- identical to rv32's, both
 * being the "generic" Linux syscall ABI shared with arm64 (see
 * numbers_riscv.h's own comment). Hand-written for now since only a
 * couple are needed.
 */

// see numbers_arm64.h's identical comment: the "generic" ABI has no
// legacy open(), only openat() -- _sysopen() in syscall_linux_riscv64.h
// bridges the gap with AT_FDCWD.
// claude: see numbers_arm64.h's identical comment -- no legacy unlink()
// in this ABI, so remove() is a shim over unlinkat() in
// syscall_linux_riscv64.h.
// claude: mkdirat -- see numbers_arm64.h; rmdir is unlinkat+AT_REMOVEDIR.
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
 * brk() (0/-1) is the shim in syscall_linux_riscv64.h. From
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
/* claude: the stat family (Tier 3) -- same generic-ABI numbers as
 * arm64 (see numbers_arm64.h's comment): fstat=80, fchmod=52,
 * ftruncate=46. NOT shared with numbers_riscv.h (riscv32): per
 * docs/claude_notes/plan_syscalls.txt, 79/80 are 64-bit-only in
 * scripts/syscall.tbl, so riscv32 has no fstat/newfstatat at all and
 * would need statx instead -- not implemented, see numbers_riscv.h.
 */
#define SYS_fstat	80
#define SYS_fchmod	52
#define SYS_ftruncate	46
/* claude: dirread's other raw call (Tier 3.5) -- same generic-ABI
 * number as arm64, getdents64=61 (see numbers_arm64.h's comment).
 */
#define SYS_getdents64	61

// renameat2(2) -- same generic-ABI number as arm64 (see
// numbers_arm64.h's comment).
#define SYS_renameat2	276
/* claude: Tier 4 process control -- same generic-ABI numbers as arm64
 * (see numbers_arm64.h's fuller comment): clone=220, execve=221,
 * wait4=260, pipe2=59. wait4's time32-vs-64 gate is open here too
 * (__BITS_PER_LONG==64 on rv64, unlike rv32 -- see numbers_riscv.h,
 * which is why THIS file is not simply reused there the way it is for
 * the read/write/openat/close/exit "common" rows). syscall_linux_
 * riscv64.h's own fork()/pipe() shims mirror arm64's.
 */
#define SYS_clone	220
#define SYS_execve	221
#define SYS_wait4	260
#define SYS_pipe2	59

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the Ksigaction/handler-shape design story (identical 64-bit
 * flags/mask widths and 1-word sigset_t). kill=129, rt_sigreturn=139,
 * rt_sigaction=134: the generic-ABI numbers, same as arm64
 * (numbers_arm64.h) and riscv32 (numbers_riscv.h) -- confirmed via
 * this project's own arm64 dev host's asm-generic/unistd.h, which
 * riscv64's own arch/riscv/include/uapi/asm/unistd.h defers to
 * entirely for this row (no per-arch override, same story
 * numbers_riscv.h's own comment already establishes for rv32).
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
 * kernel's own default worked fine). Kept as accurate reference facts. */
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	139

// rc self-hosting's Isatty() (os/linux/isatty.c) -- confirmed against
// real riscv64-linux-musl/bits/syscall.h, matching this arch's own
// SYS_kill=129 above (same reference table as numbers_arm64.h's).
#define SYS_ioctl	29
