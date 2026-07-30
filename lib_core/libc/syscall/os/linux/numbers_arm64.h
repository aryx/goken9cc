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
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
