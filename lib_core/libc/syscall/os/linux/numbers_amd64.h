/* Linux amd64 syscall numbers. Hand-written for now since only a
 * couple are needed; a kernel-header-scraping generator (like
 * GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once more
 * are needed.
 */

#define SYS_read	0
#define SYS_write	1
#define SYS_exit	60
