/* Linux riscv64 (rv64) syscall numbers -- identical to rv32's, both
 * being the "generic" Linux syscall ABI shared with arm64 (see
 * numbers_riscv.h's own comment). Hand-written for now since only a
 * couple are needed.
 */

#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
