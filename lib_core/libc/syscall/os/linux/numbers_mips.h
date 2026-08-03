/* Linux mips (o32) syscall numbers. Unlike the other archs handled so
 * far, o32 syscalls are offset by 4000 from their "base" number (see
 * arch/mips/include/uapi/asm/unistd.h upstream) -- a wrinkle specific
 * to this arch, not shared with arm64/amd64/riscv64's shared "generic"
 * numbering. Hand-written for now since only a couple are needed; a
 * kernel-header-scraping generator (like GO/pkg/syscall/mksysnum_linux.sh,
 * which itself has a separate GOARCH=mips path for exactly this reason)
 * is a natural follow-up once more are needed.
 */

#define SYS_read	4003
#define SYS_write	4004
#define SYS_open	4005
#define SYS_close	4006
#define SYS_lseek	4019
#define SYS_exit	4001
/* claude: the same legacy 10/12 as 386/arm, plus this table's 4000
 * offset -- confirmed against arch/mips/kernel/syscalls/syscall_o32.tbl
 * upstream ("10 o32 unlink", "12 o32 chdir"). See numbers_386.h's own
 * comment for why create() needs no number of its own.
 */
#define SYS_unlink	4010
#define SYS_chdir	4012
/* claude: mkdir/rmdir -- the same legacy 39/40 as 386/arm plus this
 * table's 4000 offset ("39 o32 mkdir", "40 o32 rmdir" upstream).
 * See numbers_386.h's comment for what needs them.
 */
#define SYS_mkdir	4039
#define SYS_rmdir	4040
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_o32.tbl (+4000).
 */
#define SYS_access	4033
#define SYS_dup	4041
#define SYS_dup2	4063
/* claude: brk -- see numbers_386.h's comment; shim in
 * syscall_linux_mips.h. From syscall_o32.tbl ("45 o32 brk") +4000.
 */
#define SYS_brk	4045
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
#define SYS_getpid	4020
#define SYS_getcwd	4203
#define SYS_clock_gettime	4403
#define SYS_clock_nanosleep	4407
/* claude: note the two names above are deliberately the *plain* ones
 * even though the NUMBERS are the kernel's *_time64 variants
 * (clock_gettime64 / clock_nanosleep_time64). That aliasing is the
 * point: "SYS_clock_gettime" here means "the clock_gettime on this arch
 * whose timespec is 8+8", which is 403 on a 32-bit arch and 113/228 on
 * a 64-bit one. Keeping one name lets os/linux/time.c stay a single
 * arch-independent file instead of growing an #ifdef ladder.
 */
/* claude: the stat family (Tier 3). fstat64/fchmod confirmed against
 * arch/mips/kernel/syscalls/syscall_o32.tbl (215/94, +4000 base same
 * as every other row in this file). SYS_ftruncate64 is NOT the 386/arm
 * 194 -- mips o32 renumbered it to 212 (+4000 = 4212); do not copy
 * numbers_386.h's value here. os/linux/stat_mips.c does NOT use
 * ftruncate64 despite that -- mips o32's calling convention pads a
 * 64-bit syscall argument to an aligned register pair (the same class
 * of arg-marshaling trap as the riscv32 llseek bug in
 * docs/claude_notes/plan_syscalls.txt), which needs verification
 * against the kernel's actual sys_ftruncate64() signature before
 * trusting a hand-guessed argument layout -- left as a documented gap
 * rather than a guess.
 */
#define SYS_fstat64	4215
#define SYS_fchmod	4094
#define SYS_ftruncate64	4212
