/* Linux riscv64 (rv64) syscall numbers -- identical to rv32's, both
 * being the "generic" Linux syscall ABI shared with arm64 (see
 * numbers_riscv.h's own comment). Hand-written for now since only a
 * couple are needed.
 */

// see numbers_arm64.h's identical comment: the "generic" ABI has no
// legacy open(), only openat() -- _sysopen() in syscall_linux_riscv64.h
// bridges the gap with AT_FDCWD.
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
