/* Darwin (macOS) arm64 raw BSD syscall numbers. Confirmed empirically
 * against real macOS execution while bringing up 7l's -H6 Mach-O
 * target (see tests/s/mini/hello_macos_arm64.s and
 * docs/claude_notes/notes_exec_macho.txt) before this file existed --
 * these are the same two numbers, just relocated here so libc.a's
 * generated syscall wrappers (zsyscall_darwin_arm64.c) can use them
 * too instead of every caller hand-writing the trap. Unlike Linux's
 * arm64 (asm-generic) numbers, these carry no "class" prefix: that's
 * an amd64/x86 Darwin quirk (the 0x2000000 BSD-class bit baked into
 * the syscall number itself, see numbers_amd64.h's own comment once
 * that arch is added) -- arm64 Darwin just wants the plain BSD number
 * in x16, no prefix.
 */

/* open/close/lseek added alongside exit/write -- same arch-independent
 * BSD table as numbers_amd64.h (see that file's comment for the
 * lseek=199-not-19 story, XNU-specific and not an amd64-only quirk).
 */
#define SYS_exit	1
#define SYS_read	3
#define SYS_write	4
#define SYS_open	5
#define SYS_close	6
#define SYS_lseek	199
/* claude: unlink/chdir, same arch-independent BSD table -- see
 * numbers_amd64.h's own comment for the provenance (and for why these
 * two, unlike lseek, kept their classic 10/12).
 */
#define SYS_unlink	10
#define SYS_chdir	12
