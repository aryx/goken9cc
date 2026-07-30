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
