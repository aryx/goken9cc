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
// syscall_linux_riscv.h. Unlike SYS_lseek two lines below (which is
// really llseek on this 32-bit arch, see todo.org's own entry), both of
// these are marked "common" in scripts/syscall.tbl, so rv32 and rv64
// genuinely do share them.
// claude: mkdirat -- see numbers_arm64.h; rmdir is unlinkat+AT_REMOVEDIR.
#define SYS_mkdirat	34
#define SYS_unlinkat	35
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
