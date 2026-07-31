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
#define SYS_unlinkat	35
#define SYS_chdir	49
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
