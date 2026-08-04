/* Linux riscv (rv32) syscall numbers (the "generic" Linux syscall ABI,
 * shared with arm64/riscv64 -- see asm-generic/unistd.h upstream, and
 * confirmed against tests/c/mini2/linux_riscv.s's own already-working
 * write=64/exit=93). Hand-written for now since only a couple are
 * needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 */

// see numbers_arm64.h's identical comment: the "generic" ABI has no
// legacy open(), only openat() -- _sysopen() in syscall_linux_riscv.h
// bridges the gap with AT_FDCWD.
// claude: see numbers_arm64.h's identical comment -- no legacy unlink()
// in this ABI, so remove() is a shim over unlinkat() in
// syscall_linux_riscv.h. Unlike syscall 62 below (which is llseek, not
// lseek, on this 32-bit arch -- see its own comment), both of these are
// marked "common" in scripts/syscall.tbl, so rv32 and rv64 genuinely do
// share them.
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
/* claude: 62 is llseek here, NOT lseek. scripts/syscall.tbl marks the
 * two entries at this number by width:
 *
 *   62  32  llseek   sys_llseek
 *   62  64  lseek    sys_lseek
 *
 * and this is the 32-bit one. They are not the same call: llseek takes
 * FIVE arguments (fd, offset_high, offset_low, loff_t *result, whence)
 * and returns 0, writing the resulting offset back THROUGH THE POINTER,
 * whereas rv64's lseek takes three and returns the offset directly.
 * This file was originally a verbatim copy of numbers_riscv64.h, so it
 * inherited rv64's shape and generated a 3-arg lseek() against a 5-arg
 * syscall -- see syscall_linux_riscv.h's lseek() for the shim that
 * bridges it, and notes_arch_riscv.txt for how the bug hid.
 */
#define SYS_llseek	62
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
 * brk() (0/-1) is the shim in syscall_linux_riscv.h. From
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
/* claude: on THIS arch the time64 forms are not merely preferable, they
 * are the only option -- the third time rv32 diverges from the riscv64
 * file it was copied from (see SYS_llseek above for the first).
 * gettimeofday (169) and nanosleep (101) exist only under the `time32`
 * ABI, and rv32 is not in it: include/uapi/asm-generic/unistd.h guards
 * both with
 *     #if defined(__ARCH_WANT_TIME32_SYSCALLS) || __BITS_PER_LONG != 32
 * and arch/riscv/include/asm/unistd.h never defines that macro (riscv
 * postdates the y2038 cleanup). Calling 169 or 101 here would land on
 * whatever else occupies those numbers, or on nothing at all.
 */
/* claude: the stat family (Tier 3, docs/claude_notes/plan_syscalls.txt)
 * is a deliberate GAP on this arch, not an oversight -- no SYS_fstat
 * here. scripts/syscall.tbl marks fstat/newfstatat (79/80) as 64-bit
 * only; rv32, like every "new" 32-bit port, has no fstat/stat/lstat
 * syscalls at all, only statx. os/linux/ has no stat_riscv.c as a
 * result (see lib_core/libc/mkfile's STATOFILES), so dirfstat/dirfwstat
 * are unavailable on this one arch until statx support is written.
 */
/* claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt)
 * is left out of this round for this one arch, worth recording
 * precisely since fork and wait land on opposite sides of the same
 * time32/64 split that already bit stat above. clone=220 and
 * execve=221 are unconditional in scripts/syscall.tbl ("common"), so
 * fork (via clone(SIGCHLD,...), same as arm64/riscv64) and exec would
 * work fine here. wait4, however, is gated exactly like stat's
 * fstat/newfstatat:
 *
 *   260  time32  wait4  sys_wait4  compat_sys_wait4
 *   260  64      wait4  sys_wait4
 *
 * and asm-generic/unistd.h's own guard on __NR_wait4 is
 * `#if defined(__ARCH_WANT_TIME32_SYSCALLS) || __BITS_PER_LONG != 32`
 * -- the exact condition this file's own clock_gettime/nanosleep
 * comment above already established riscv never defines. So rv32 has
 * no wait4 at all, only waitid (95, unconditional) -- a different
 * struct (siginfo_t, not a plain int status + rusage), needing its own
 * decode logic, not a drop-in. Rather than build fork/exec alone
 * without wait to pair them, this whole tier is deferred here as one
 * gap, same "documented, not silently dropped" treatment as stat's.
 */
