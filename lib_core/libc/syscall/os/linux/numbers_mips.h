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
#define SYS_exit	4001
