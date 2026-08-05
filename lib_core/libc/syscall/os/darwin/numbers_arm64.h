/* Darwin (macOS) arm64 raw BSD syscall numbers. Confirmed empirically
 * against real macOS execution while bringing up 7l's -H6 Mach-O
 * target (see tests/s/mini/hello_macos_arm64.s and
 * docs/claude_notes/notes_exec_macho.txt) before this file existed --
 * these are the same two numbers, just relocated here so libc.a's
 * generated syscall wrappers (zsyscall_darwin_arm64.c) can use them
 * too instead of every caller hand-writing the trap. Unlike Linux's
 * arm64 (asm-generic) numbers, these carry no "class" prefix: that's
 * an amd64/x86 Darwin quirk (the 0x2000000 BSD-class bit baked into
 * the syscall number itself, see numbers_amd64.h's own comment once
 * that arch is added) -- arm64 Darwin just wants the plain BSD number
 * in x16, no prefix.
 */

/* open/close/lseek added alongside exit/write -- same arch-independent
 * BSD table as numbers_amd64.h (see that file's comment for the
 * lseek=199-not-19 story, XNU-specific and not an amd64-only quirk).
 */
#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	199
/* claude: unlink/chdir, same arch-independent BSD table -- see
 * numbers_amd64.h's own comment for the provenance (and for why these
 * two, unlike lseek, kept their classic 10/12).
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir -- see numbers_amd64.h's comment. */
#define SYS_mkdir	136
#define SYS_rmdir	137
/* claude: access/dup/dup2 -- confirmed against
 * GO/pkg/syscall/zsysnum_darwin_amd64.go (SYS_ACCESS = 33, SYS_DUP = 41,
 * SYS_DUP2 = 90). access and dup kept their classic BSD numbers, dup2
 * did not. See numbers_386.h for why dup needs both forms.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	90
/* claude: mmap -- darwin's stand-in for brk, which every other GOOS here
 * uses to back sbrk(). There is no usable brk on modern macOS at all
 * (SYS_break=17 survives in the table but fails), so os/darwin/sbrk.c
 * hands out a fresh MAP_ANON region per call instead of moving a break;
 * see that file, and lib_core/libc/mkfile's SBRKOFILES for why
 * port/sbrk.c is not built here. From GO/pkg/syscall/zsysnum_darwin_amd64.go
 * ("SYS_MMAP = 197"), the same 2010-era snapshot the rest of this
 * directory's numbers come from -- the BSD table is arch-independent, so
 * it carries over to arm64 unchanged (unlike the stat rows, which do
 * not; see docs/claude_notes/plan_syscalls.txt).
 * munmap (73) is deliberately NOT here: nothing frees an sbrk region.
 */
#define SYS_mmap	197
/* claude: the "small tier" -- getpid, getwd, time/nsec, sleep. All four
 * verified against XNU's own bsd/kern/syscalls.master, not the 2010 Go
 * snapshot the rest of this file came from, because two of them are not
 * in that snapshot at all and the plan file's guess for one was wrong.
 *
 * getpid (20) and gettimeofday (116) are straightforward. The other two
 * are not, and both are cases where Darwin simply has no equivalent of
 * the Linux syscall:
 *
 * - There is NO getcwd syscall on Darwin, under any name. (Note
 *   docs/claude_notes/plan_syscalls.txt guessed __getcwd = 296 and
 *   flagged it unverified; 296 is in fact vm_pressure_monitor.) So
 *   getwd is built on fcntl(F_GETPATH) instead -- open the directory,
 *   ask the kernel that descriptor's path. This is the same idea as
 *   Plan9's fd2path, which is why os/darwin/getwd.c and
 *   os/plan9/getwd.c look alike and neither looks like linux's.
 * - There is NO nanosleep syscall either; Darwin implements it in
 *   libSystem over __semwait_signal. select(2) with only a timeout is
 *   the classic portable substitute and is a real syscall here, so
 *   os/darwin/time.c uses that.
 *
 * gettimeofday is marked NO_SYSCALL_STUB in syscalls.master, meaning
 * libSystem exports no automatic wrapper -- irrelevant to us, since we
 * trap directly, but it explains why it looks unusual there. Its third
 * argument (uint64_t *mach_absolute_time) is a Darwin addition; passing
 * nil asks for just the timeval.
 */
#define SYS_getpid	20
#define SYS_fcntl	92
#define SYS_select	93
#define SYS_gettimeofday	116
/* claude: the stat family (Tier 3) -- see numbers_amd64.h's identical
 * comment. The original guess here was plain fstat=189 on the theory
 * that arm64 macOS postdates XNU's stat64 unification and so the plain
 * syscall would already return the unified struct -- WRONG, disproven
 * empirically on a real macOS 26/Darwin 25 arm64 host: trap 189 still
 * returns the legacy narrow layout (dev, 32-bit ino, mode, nlink, ...,
 * no birthtime), while stat_arm64.c's Kstat assumes the same unified
 * shape stat_amd64.c does (dev, mode, nlink, 64-bit ino, ..., four
 * timespec pairs, birthtime included). Confirmed by dumping raw fstat
 * output byte-for-byte against a clang-compiled reference binary's
 * libc fstat() on the same fd: trap 339 (SYS_fstat64, same number as
 * amd64 -- this part of the BSD table really is arch-independent)
 * matches the reference exactly; trap 189 does not. Kept the
 * SYS_fstat64 name (not SYS_fstat) to match numbers_amd64.h and
 * syscall_darwin_arm64.decl's own reference to it.
 * fchmod/ftruncate are the arch-independent classic-BSD numbers.
 */
#define SYS_fstat64	339
#define SYS_fchmod	124
#define SYS_ftruncate	201
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * Confirmed against a local checkout of XNU's own bsd/kern/
 * syscalls.master (apple-oss-distributions/xnu, not the 2010 Go
 * snapshot this file otherwise leans on -- same primary-source
 * discipline as the stat family's own re-verification above):
 *
 *   2   AUE_FORK    { int fork(void) NO_SYSCALL_STUB; }
 *   7   AUE_WAIT4   { int wait4(int pid, user_addr_t status,
 *                                int options, user_addr_t rusage)
 *                                NO_SYSCALL_STUB; }
 *   42  AUE_PIPE    { int pipe(void); }
 *   59  AUE_EXECVE  { int execve(char *fname, char **argp, char **envp); }
 *
 * NO_SYSCALL_STUB on fork/wait4 means libSystem exports no automatic
 * wrapper for either -- Apple's own Libsyscall hand-writes custom
 * assembly for both, which is the same conclusion the classic-BSD
 * dual-register-return convention already implied (see
 * syscall_darwin_arm64.h's own fork()/pipe() trampoline comments): a
 * plain _syscall6-style stub cannot express either correctly.
 * pipe(void) takes NO ARGUMENTS AT ALL -- not even the pointer POSIX's
 * own pipe(2) prototype suggests -- confirming both fds really do come
 * back purely through registers, with nothing else to pass in.
 * Arch-independent BSD table, so amd64 shares all four numbers.
 */
#define SYS_fork	2
#define SYS_wait4	7
#define SYS_pipe	42
#define SYS_execve	59

/* claude: Tier 6 notification -- see numbers_amd64.h's fuller comment
 * for the postnote()-only scope and why notify()/noted() are
 * deliberately not attempted here. kill=37, same 3-arg BSD signature,
 * arch-independent BSD table so this arch shares the number.
 */
#define SYS_kill	37

/* claude: rc self-hosting's Isatty()/Readdir()/Opendir() -- see
 * numbers_amd64.h's fuller comment (SYS_ioctl/SYS_getdirentries64
 * arch-independent: same BSD syscall table). TIOCGETA itself is
 * defined locally in os/darwin/isatty.c, not here -- see that
 * file's own header comment. */
#define SYS_ioctl	54
#define SYS_getdirentries64	344
