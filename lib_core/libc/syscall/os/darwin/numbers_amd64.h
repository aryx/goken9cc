/* Darwin (macOS) amd64 raw BSD syscall numbers -- same numbers as
 * darwin/arm64's (numbers_arm64.h): XNU's BSD syscall table is
 * arch-independent, only the trap encoding differs per arch (arm64:
 * SVC $0x80 with the number in R16; amd64: SYSCALL with the number in
 * AX plus a 0x2000000 "BSD class" prefix -- applied in svc_amd64.s,
 * not baked in here, so these stay the same bare numbers on both
 * arches). Confirmed empirically against real macOS execution while
 * bringing up 6l's -H6 Mach-O target (see
 * docs/claude_notes/notes_exec_macho.txt and
 * tests/s/mini/hello_macos_amd64.s, which predates this file).
 */

/* open/close/lseek added alongside exit/write: same BSD table
 * (confirmed against GO/pkg/syscall/zsysnum_darwin_amd64.go, the same
 * 2010-era Go snapshot the rest of this file's siblings pattern
 * themselves on -- see docs/claude_notes/notes_libc_selfhost.txt).
 * lseek is 199, NOT the classic-Unix 19 that Linux/386's and Linux/arm's
 * numbers use for the same call -- XNU renumbered several of its BSD
 * syscalls when it added "quad" (64-bit off_t) syscall variants; 19 is
 * still taken by old_lseek in XNU's table, so plain lseek moved.
 */
#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	199
