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
/* claude: unlink/chdir back Plan9's remove()/chdir() (include/os/dir.h).
 * Unlike lseek above, XNU never renumbered these two: they're still the
 * classic-BSD 10/12, the same numbers Linux/386 and Linux/arm use --
 * confirmed against GO/pkg/syscall/zsysnum_darwin_amd64.go
 * (SYS_UNLINK = 10, SYS_CHDIR = 12), the same 2010-era snapshot the
 * numbers above came from. Still the arch-independent BSD table, so
 * numbers_arm64.h repeats them verbatim. See
 * syscall/os/linux/numbers_386.h's own comment for why create() needs
 * no syscall number of its own.
 */
#define SYS_unlink	10
#define SYS_chdir	12
/* claude: mkdir/rmdir, for create()'s DMDIR bit and remove()'s
 * directory case. Unlike unlink/chdir above these are NOT the classic
 * low numbers -- confirmed against
 * GO/pkg/syscall/zsysnum_darwin_amd64.go (SYS_MKDIR = 136,
 * SYS_RMDIR = 137). Still the arch-independent BSD table.
 */
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
