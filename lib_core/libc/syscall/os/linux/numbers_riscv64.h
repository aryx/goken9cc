/* Linux riscv64 (rv64) syscall numbers -- identical to rv32's, both
 * being the "generic" Linux syscall ABI shared with arm64 (see
 * numbers_riscv.h's own comment). Hand-written for now since only a
 * couple are needed.
 */

// see numbers_arm64.h's identical comment: the "generic" ABI has no
// legacy open(), only openat() -- _sysopen() in syscall_linux_riscv64.h
// bridges the gap with AT_FDCWD.
// claude: see numbers_arm64.h's identical comment -- no legacy unlink()
// in this ABI, so remove() is a shim over unlinkat() in
// syscall_linux_riscv64.h.
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
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
