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
/* claude: mkdir/rmdir -- see numbers_amd64.h's comment. */
#define SYS_mkdir	136
#define SYS_rmdir	137
/* claude: access/dup/dup2 -- confirmed against
 * GO/pkg/syscall/zsysnum_darwin_amd64.go (SYS_ACCESS = 33, SYS_DUP = 41,
 * SYS_DUP2 = 90). access and dup kept their classic BSD numbers, dup2
 * did not. See numbers_386.h for why dup needs both forms.
 */
#define SYS_access	33
#define SYS_dup	41
#define SYS_dup2	90
/* claude: mmap -- darwin's stand-in for brk, which every other GOOS here
 * uses to back sbrk(). There is no usable brk on modern macOS at all
 * (SYS_break=17 survives in the table but fails), so os/darwin/sbrk.c
 * hands out a fresh MAP_ANON region per call instead of moving a break;
 * see that file, and lib_core/libc/mkfile's SBRKOFILES for why
 * port/sbrk.c is not built here. From GO/pkg/syscall/zsysnum_darwin_amd64.go
 * ("SYS_MMAP = 197"), the same 2010-era snapshot the rest of this
 * directory's numbers come from -- the BSD table is arch-independent, so
 * it carries over to arm64 unchanged (unlike the stat rows, which do
 * not; see docs/claude_notes/plan_syscalls.txt).
 * munmap (73) is deliberately NOT here: nothing frees an sbrk region.
 */
#define SYS_mmap	197
