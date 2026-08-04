/* Linux 386 syscall numbers (the classic/legacy i386 numbering, not
 * the "generic" one arm64/riscv/riscv64 share -- confirmed against
 * tests/c/mini2/linux_386.s's own already-working write=4/exit=1).
 * Hand-written for now since only a couple are needed; a kernel-
 * header-scraping generator (like GO/pkg/syscall/mksysnum_linux.sh)
 * is a natural follow-up once more are needed.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h)
 * -- read straight off arch/x86/entry/syscalls/syscall_32.tbl upstream
 * ("10 i386 unlink", "12 i386 chdir"), not from memory. arm and mips
 * share these two numbers (see their own numbers_*.h); amd64 does not.
 * No SYS_creat here even though this table has one (8): Plan9's
 * create() needs an fd opened in a caller-chosen mode, which creat(2)
 * (always write-only) can't express -- os/linux/open.c builds it out of
 * open(O_CREAT|O_TRUNC) instead, so it costs no syscall number at all.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir, for Plan9 create()'s DMDIR bit and remove()'s
 * ability to delete a directory (which POSIX unlink(2) refuses --
 * see port/remove.c). Same syscall_32.tbl rows: "39 i386 mkdir",
 * "40 i386 rmdir". arm and mips share these two as well.
 */
#define SYS_mkdir	39
#define SYS_rmdir	40
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_32.tbl.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	63
/* claude: brk -- the raw kernel primitive port/sbrk.c is built on (it
 * is sbrk, not brk, that the toolchain's own callers use; see
 * include/os/mem.h and port/sbrk.c). Note Linux's brk(2) does NOT use
 * the usual negative-errno convention: it returns the NEW break, and
 * signals failure by returning the UNCHANGED old one. The public
 * Plan9-shaped brk() (0/-1) is the shim in syscall_linux_386.h.
 * From syscall_32.tbl ("45 i386 brk"); arm shares this number.
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
/* claude: the stat family (Tier 3, docs/claude_notes/plan_syscalls.txt).
 * *64 forms are mandatory on every 32-bit Linux arch here -- the plain
 * fstat/ftruncate would silently truncate ino_t/off_t, the same class
 * of bug as the lseek/vlong one fixed in 8bbdbab5a. Confirmed against
 * arch/x86/entry/syscalls/syscall_32.tbl. fchmod needs no *64 variant
 * (its argument is a plain mode_t, not an offset/inode).
 */
#define SYS_fstat64	197
#define SYS_fchmod	94
#define SYS_ftruncate64	194
/* claude: dirread's two raw calls (Tier 3.5). Confirmed against
 * arch/x86/entry/syscalls/syscall_32.tbl: openat=295, getdents64=220 --
 * NOT the same numbers as amd64/arm below, this arch's table renumbered
 * both independently as usual.
 */
#define SYS_openat	295
#define SYS_getdents64	220
// renameat2(2) -- see numbers_amd64.h's own comment for why this,
// not plain rename/renameat, is used uniformly across all 6 arches.
#define SYS_renameat2	353
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
 * Confirmed against a local checkout of the kernel source
 * (arch/x86/entry/syscalls/syscall_32.tbl) rather than transcribed from
 * the plan doc's own table: "2 i386 fork sys_fork", "11 i386 execve
 * sys_execve", "42 i386 pipe sys_pipe", "114 i386 wait4 sys_wait4".
 * fork/execve/wait4/pipe all reach the public Plan9-shaped
 * name through a port/*.c bridge (port/fork.c, port/exec.c, port/wait.c,
 * port/pipe.c) rather than being decl-generated straight to it, unlike
 * getpid/chdir/access above -- fork()/exec()/wait() all need the raw
 * negative-errno result normalized to Plan9's exact -1 on failure (see
 * port/fork.c's own comment for why this matters for a real caller,
 * compilers/pcc/pcc.c's `switch(fork()){case -1: ...}`).
 */
#define SYS_fork	2
#define SYS_execve	11
#define SYS_wait4	114
#define SYS_pipe	42
