/* Linux amd64 syscall numbers. Hand-written for now since only a
 * couple are needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 *
 * open/close/lseek confirmed against GO/pkg/syscall/zsysnum_linux_amd64.go
 * (SYS_OPEN=2, SYS_CLOSE=3, SYS_LSEEK=8), the same 2010-era Go snapshot
 * this project's syscall/ layer already patterns itself on -- see
 * docs/claude_notes/notes_libc_selfhost.txt.
 */

#define SYS_read	0
#define SYS_write	1
#define SYS_open	2
#define SYS_close	3
#define SYS_lseek	8
#define SYS_exit	60
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h).
 * This arch renumbered everything above open/close/lseek, so these are
 * NOT 386's 10/12 -- read straight off
 * arch/x86/entry/syscalls/syscall_64.tbl upstream ("87 common unlink",
 * "80 common chdir"). See numbers_386.h's own comment for why create()
 * needs no syscall number of its own on any Unix here.
 */
#define SYS_chdir	80
#define SYS_unlink	87
/* claude: mkdir/rmdir, again NOT 386's 39/40 -- "83 common mkdir",
 * "84 common rmdir" in syscall_64.tbl. See numbers_386.h's comment for
 * what needs them.
 */
#define SYS_mkdir	83
#define SYS_rmdir	84
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_64.tbl.
 */
#define SYS_access	21
#define SYS_dup	32
#define SYS_dup2	33
/* claude: brk -- see numbers_386.h's comment for the return-convention
 * story (the shim is in syscall_linux_amd64.h). From syscall_64.tbl
 * ("12 common brk"), one of the many rows where amd64 renumbered.
 */
#define SYS_brk	12
