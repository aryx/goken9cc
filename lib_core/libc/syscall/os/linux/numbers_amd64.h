/* Linux amd64 syscall numbers. Hand-written for now since only a
 * couple are needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 *
 * open/close/lseek confirmed against GO/pkg/syscall/zsysnum_linux_amd64.go
 * (SYS_OPEN=2, SYS_CLOSE=3, SYS_LSEEK=8), the same 2010-era Go snapshot
 * this project's syscall/ layer already patterns itself on -- see
 * docs/claude_notes/notes_libc_selfhost.txt.
 */

#define SYS_read	0
#define SYS_write	1
#define SYS_open	2
#define SYS_close	3
#define SYS_lseek	8
#define SYS_exit	60
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h).
 * This arch renumbered everything above open/close/lseek, so these are
 * NOT 386's 10/12 -- read straight off
 * arch/x86/entry/syscalls/syscall_64.tbl upstream ("87 common unlink",
 * "80 common chdir"). See numbers_386.h's own comment for why create()
 * needs no syscall number of its own on any Unix here.
 */
#define SYS_chdir	80
#define SYS_unlink	87
/* claude: mkdir/rmdir, again NOT 386's 39/40 -- "83 common mkdir",
 * "84 common rmdir" in syscall_64.tbl. See numbers_386.h's comment for
 * what needs them.
 */
#define SYS_mkdir	83
#define SYS_rmdir	84
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_64.tbl.
 */
#define SYS_access	21
#define SYS_dup	32
#define SYS_dup2	33
/* claude: brk -- see numbers_386.h's comment for the return-convention
 * story (the shim is in syscall_linux_amd64.h). From syscall_64.tbl
 * ("12 common brk"), one of the many rows where amd64 renumbered.
 */
#define SYS_brk	12
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
#define SYS_getpid	39
#define SYS_getcwd	79
#define SYS_clock_gettime	228
#define SYS_clock_nanosleep	230
/* claude: the stat family (Tier 3). Confirmed against
 * arch/x86/entry/syscalls/syscall_64.tbl: fstat=5, fchmod=91,
 * ftruncate=77 -- amd64 is 64-bit-native throughout, so unlike
 * 386/arm/mips there is no *64 variant to reach for.
 */
#define SYS_fstat	5
#define SYS_fchmod	91
#define SYS_ftruncate	77
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against
 * arch/x86/entry/syscalls/syscall_64.tbl: openat=257, getdents64=217.
 */
#define SYS_openat	257
#define SYS_getdents64	217
// renameat2(2) (Linux 3.15+, arch/x86/entry/syscalls/syscall_64.tbl) --
// used uniformly across all 6 arches instead of the older plain
// rename(2)/renameat(2) (both also available on this arch) so
// os/linux/dirwstat.c needs no per-arch fallback: renameat2 is the
// only rename-family syscall confirmed present on every arch here,
// arm64/riscv64 included (their generic unistd.h only gates legacy
// renameat behind __ARCH_WANT_RENAMEAT, which they don't define --
// same reasoning as their already-openat-only/unlinkat-only story).
#define SYS_renameat2	316
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * Confirmed against a local checkout of arch/x86/entry/syscalls/
 * syscall_64.tbl: "57 common fork sys_fork", "59 64 execve sys_execve"
 * (64 here means "only under the native 64-bit ABI, not x32" -- this
 * project never targets x32, so that is not a caveat that affects us),
 * "61 common wait4 sys_wait4", "22 common pipe sys_pipe". See
 * numbers_386.h's comment for why these route through port/*.c rather
 * than straight to their public name.
 */
#define SYS_fork	57
#define SYS_execve	59
#define SYS_wait4	61
#define SYS_pipe	22

/* claude: Tier 6 notification (docs/claude_notes/plan_syscalls.txt,
 * docs/claude_notes/notes_libc_api_design.txt's "Notes vs. signals").
 * Confirmed against a real checkout of arch/x86/entry/syscalls/
 * syscall_64.tbl ("13 common rt_sigaction sys_rt_sigaction", "15
 * common rt_sigreturn sys_rt_sigreturn", "62 common kill sys_kill").
 *
 * Ksigaction is the raw kernel-ABI struct rt_sigaction(2) actually
 * reads/writes -- NOT glibc's struct sigaction (different layout).
 * Deliberately the SIMPLE (non-SA_SIGINFO) handler shape, `void
 * (*)(int)`, matching plan9port's own BOOT/lib9/notify.c (confirmed
 * real, working precedent, not guessed) -- os/linux/notify.c's own
 * dispatcher only ever needs the signal NUMBER (to look up a note
 * string), never siginfo_t/ucontext_t, so there is nothing to gain
 * from the 3-argument form and a real category of struct-layout
 * mistakes to lose by using it.
 *
 * Field widths matter more here than in most raw structs in this
 * tree: this compiler's own `long`/`ulong` are ALWAYS 4 bytes even on
 * 64-bit arches (see numbers_amd64.h's sibling files' own comments,
 * and os/linux/stat_amd64.c's identical warning for Kstat) -- but the
 * real kernel struct's sa_flags/sa_mask fields are the ARCH's native
 * word width, 8 bytes here. So flags/mask are uvlong, matching
 * Kstat's own explicit-width discipline, not the bare ulong that
 * would silently misalign restorer/mask by 4 bytes each. mask is a
 * SINGLE uvlong (not an array): amd64's kernel sigset_t is
 * _NSIG_WORDS=1 (_NSIG=64, BITS_PER_LONG=64) -- confirmed empirically
 * on this project's own arm64 host (identical generic-ABI sigset
 * story, see numbers_arm64.h) and cross-checked against mips's very
 * different _NSIG=128/_NSIG_WORDS=4 story (numbers_mips.h) to make
 * sure this isn't just assumed uniform across every arch.
 */
typedef struct Ksigaction Ksigaction;
struct Ksigaction {
	void	(*handler)(int);
	uvlong	flags;
	void	(*restorer)(void);
	uvlong	mask;
};
#define SYS_kill	62
#define SYS_rt_sigaction	13
#define SA_RESTORER_VAL	0x04000000
#define __NR_rt_sigreturn	15

// rc self-hosting's Isatty() (os/linux/isatty.c) -- confirmed against
// real x86_64-linux-musl/bits/syscall.h, cross-checked against this
// arch's own SYS_kill=62 above matching the same reference table.
#define SYS_ioctl	16
