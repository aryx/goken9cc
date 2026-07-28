/* Linux riscv (rv32) syscall numbers (the "generic" Linux syscall ABI,
 * shared with arm64/riscv64 -- see asm-generic/unistd.h upstream, and
 * confirmed against tests/c/mini2/linux_riscv.s's own already-working
 * write=64/exit=93). Hand-written for now since only a couple are
 * needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 */

#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
