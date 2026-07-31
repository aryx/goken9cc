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
/* claude: mkdir/rmdir, for Plan9 create()'s DMDIR bit and remove()'s
 * ability to delete a directory (which POSIX unlink(2) refuses --
 * see port/remove.c). Same syscall_32.tbl rows: "39 i386 mkdir",
 * "40 i386 rmdir". arm and mips share these two as well.
 */
#define SYS_mkdir	39
#define SYS_rmdir	40
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_32.tbl.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	63
