/* Linux mips (o32) syscall numbers. Unlike the other archs handled so
 * far, o32 syscalls are offset by 4000 from their "base" number (see
 * arch/mips/include/uapi/asm/unistd.h upstream) -- a wrinkle specific
 * to this arch, not shared with arm64/amd64/riscv64's shared "generic"
 * numbering. Hand-written for now since only a couple are needed; a
 * kernel-header-scraping generator (like GO/pkg/syscall/mksysnum_linux.sh,
 * which itself has a separate GOARCH=mips path for exactly this reason)
 * is a natural follow-up once more are needed.
 */

#define SYS_read	4003
#define SYS_write	4004
#define SYS_open	4005
#define SYS_close	4006
#define SYS_lseek	4019
#define SYS_exit	4001
/* claude: the same legacy 10/12 as 386/arm, plus this table's 4000
 * offset -- confirmed against arch/mips/kernel/syscalls/syscall_o32.tbl
 * upstream ("10 o32 unlink", "12 o32 chdir"). See numbers_386.h's own
 * comment for why create() needs no number of its own.
 */
#define SYS_unlink	4010
#define SYS_chdir	4012
/* claude: mkdir/rmdir -- the same legacy 39/40 as 386/arm plus this
 * table's 4000 offset ("39 o32 mkdir", "40 o32 rmdir" upstream).
 * See numbers_386.h's comment for what needs them.
 */
#define SYS_mkdir	4039
#define SYS_rmdir	4040
/* claude: access/dup/dup2. Plan9's access() mode bits (AEXIST=0,
 * AEXEC=1, AWRITE=2, AREAD=4 -- include/os/file.h) happen to equal
 * POSIX's F_OK/X_OK/W_OK/R_OK exactly, so access needs no
 * translation at all. Plan9's dup(old,new) needs BOTH of the other
 * two: new==-1 means "lowest free fd" (plain dup), anything else is
 * dup2. See port/dup.c. From syscall_o32.tbl (+4000).
 */
#define SYS_access	4033
#define SYS_dup	4041
#define SYS_dup2	4063
