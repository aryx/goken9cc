/* Darwin (macOS) amd64 raw BSD syscall numbers -- same numbers as
 * darwin/arm64's (numbers_arm64.h): XNU's BSD syscall table is
 * arch-independent, only the trap encoding differs per arch (arm64:
 * SVC $0x80 with the number in R16; amd64: SYSCALL with the number in
 * AX plus a 0x2000000 "BSD class" prefix -- applied in svc_amd64.s,
 * not baked in here, so these stay the same bare numbers on both
 * arches). Confirmed empirically against real macOS execution while
 * bringing up 6l's -H6 Mach-O target (see
 * docs/claude_notes/notes_exec_macho.txt and
 * tests/s/mini/hello_macos_amd64.s, which predates this file).
 */

/* open/close/lseek added alongside exit/write: same BSD table
 * (confirmed against GO/pkg/syscall/zsysnum_darwin_amd64.go, the same
 * 2010-era Go snapshot the rest of this file's siblings pattern
 * themselves on -- see docs/claude_notes/notes_libc_selfhost.txt).
 * lseek is 199, NOT the classic-Unix 19 that Linux/386's and Linux/arm's
 * numbers use for the same call -- XNU renumbered several of its BSD
 * syscalls when it added "quad" (64-bit off_t) syscall variants; 19 is
 * still taken by old_lseek in XNU's table, so plain lseek moved.
 */
#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	199
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h).
 * Unlike lseek above, XNU never renumbered these two: they're still the
 * classic-BSD 10/12, the same numbers Linux/386 and Linux/arm use --
 * confirmed against GO/pkg/syscall/zsysnum_darwin_amd64.go
 * (SYS_UNLINK = 10, SYS_CHDIR = 12), the same 2010-era snapshot the
 * numbers above came from. Still the arch-independent BSD table, so
 * numbers_arm64.h repeats them verbatim. See
 * syscall/os/linux/numbers_386.h's own comment for why create() needs
 * no syscall number of its own.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir, for create()'s DMDIR bit and remove()'s
 * directory case. Unlike unlink/chdir above these are NOT the classic
 * low numbers -- confirmed against
 * GO/pkg/syscall/zsysnum_darwin_amd64.go (SYS_MKDIR = 136,
 * SYS_RMDIR = 137). Still the arch-independent BSD table.
 */
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
/* claude: the stat family (Tier 3, docs/claude_notes/plan_syscalls.txt).
 * Confirmed against apple-oss-distributions/xnu's bsd/kern/syscalls.master
 * (same primary-source discipline as the getwd/time comment above, not
 * the 2010 Go snapshot -- that snapshot predates arm64 macOS entirely,
 * see plan_syscalls.txt's own caveat on trusting it for this family).
 * fstat64=339 here is NOT numbers_arm64.h's fstat=189: XNU unified the
 * two stat ABIs at different times per arch (arm64 macOS postdates the
 * unification, amd64 does not), so the plain and *64 syscalls diverged
 * -- but both land in the SAME struct shape in stat_amd64.c/
 * stat_arm64.c, only the syscall NUMBER differs. fchmod/ftruncate are
 * the arch-independent classic-BSD numbers, same on both arches.
 */
#define SYS_fstat64	339
#define SYS_fchmod	124
#define SYS_ftruncate	201
/* claude: Tier 4 process control -- see numbers_arm64.h's identical
 * comment (confirmed against the same XNU syscalls.master, and the
 * arch-independent BSD table means this arch shares all four numbers
 * with arm64's).
 */
#define SYS_fork	2
#define SYS_wait4	7
#define SYS_pipe	42
#define SYS_execve	59

/* claude: Tier 6 notification (docs/claude_notes/plan_syscalls.txt) --
 * postnote() ONLY, not the full tier. kill=37 confirmed against this
 * project's own vendored 2010 Go snapshot (zsysnum_darwin_amd64.go:
 * "SYS_KILL = 37 // { int kill(int pid, int signum, int posix); }"),
 * a real BSD 3-argument kill unlike Linux's plain 2-argument one --
 * the third `posix` argument is a legacy BSD-vs-POSIX-semantics
 * switch; the same snapshot's own syscall_darwin.go shows the real Go
 * runtime always passes 1 (`func Kill(pid, signum int) { return
 * kill(pid, signum, 1) }`), which os/darwin/postnote.c does too.
 * Signal numbers for this tier's curated set (HUP=1/INT=2/QUIT=3/
 * PIPE=13/ALRM=14/TERM=15, zerrors_darwin_amd64.go) are numerically
 * identical to Linux's non-mips arches -- same classic-BSD heritage,
 * confirmed rather than assumed given how much this project's own
 * mips work already diverged from that assumption elsewhere.
 *
 * notify()/noted() (real signal HANDLER installation, i.e. sigaction
 * itself) are deliberately NOT implemented here -- see os/darwin/
 * postnote.c's own header comment for why: XNU's raw sigaction(2)
 * needs a real userspace sa_tramp trampoline (struct __sigaction, not
 * the plain struct sigaction glibc-alike shape every other GOOS in
 * this tree gets away with), and public sources leave genuine
 * ambiguity about whether that trampoline is still mandatory on
 * modern XNU (a newer SA_USERTRAMP flag suggests the kernel can
 * supply its own instead) or version-dependent -- unlike every other
 * gap in this tree, this is not just "unverified without hardware",
 * it is "the design itself isn't confidently resolved from public
 * sources available here". Left as a real, documented gap rather than
 * shipped as unverifiable guessed trampoline assembly.
 */
#define SYS_kill	37

/* claude: rc self-hosting's Isatty() (os/darwin/isatty.c) and
 * Readdir()/Opendir() (os/darwin/dirread.c) -- confirmed against this
 * project's own vendored GO/pkg/syscall/zsysnum_darwin_amd64.go, the
 * same 2010-era snapshot every other number in this file traces to
 * ("SYS_IOCTL = 54", "SYS_GETDIRENTRIES64 = 344 ... NO_SYSCALL_STUB").
 * Arch-independent BSD table, same numbers on arm64.
 *
 * TIOCGETA (isatty.c's own ioctl request) is NOT here, unlike SYS_kill
 * and friends -- it's a request-code encoding, not a syscall number,
 * so nothing generates a wrapper from it the way mksyscall.sh does for
 * the SYS_* names above; isatty.c defines it locally instead, the same
 * way os/linux/isatty.c defines TCGETS locally rather than pulling it
 * from a numbers_$cputype.h (this file is never #include'd by any .c,
 * only read by mksyscall.sh's own code generation).
 */
#define SYS_ioctl	54
#define SYS_getdirentries64	344

// alarm() (os/darwin/alarm.c). BSD number, like every other entry
// here -- 83 = setitimer, 86 = getitimer in XNU's own
// bsd/kern/syscalls.master. Unlike linux, this GOOS does have a
// SYS_alarm-free table anyway: BSD never carried one, alarm(3) is a
// libc wrapper over setitimer there too.
#define SYS_setitimer	83
