/* Linux 386 syscall numbers (the classic/legacy i386 numbering, not
 * the "generic" one arm64/riscv/riscv64 share -- confirmed against
 * tests/c/mini2/linux_386.s's own already-working write=4/exit=1).
 * Hand-written for now since only a couple are needed; a kernel-
 * header-scraping generator (like GO/pkg/syscall/mksysnum_linux.sh)
 * is a natural follow-up once more are needed.
 */

#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	19
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h)
 * -- read straight off arch/x86/entry/syscalls/syscall_32.tbl upstream
 * ("10 i386 unlink", "12 i386 chdir"), not from memory. arm and mips
 * share these two numbers (see their own numbers_*.h); amd64 does not.
 * No SYS_creat here even though this table has one (8): Plan9's
 * create() needs an fd opened in a caller-chosen mode, which creat(2)
 * (always write-only) can't express -- os/linux/open.c builds it out of
 * open(O_CREAT|O_TRUNC) instead, so it costs no syscall number at all.
 */
#define SYS_unlink	10
#define SYS_chdir	12
