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
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
