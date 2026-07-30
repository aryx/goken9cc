/* Linux arm (EABI, 32-bit) syscall numbers (the classic/legacy
 * numbering, not the "generic" one arm64/riscv/riscv64 share --
 * confirmed against tests/c/mini2/linux_arm.s's own already-working
 * write=4/exit=1, same numbers as 386's). Hand-written for now since
 * only a couple are needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
