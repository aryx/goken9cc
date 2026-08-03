/* Linux arm64 syscall numbers (the "generic" Linux syscall ABI, shared
 * with riscv64 -- see asm-generic/unistd.h upstream). Hand-written for
 * now since only a couple are needed; a kernel-header-scraping generator
 * (like GO/pkg/syscall/mksysnum_linux.sh) is a natural follow-up once
 * more are needed.
 */

// no legacy SYS_open in the "generic" Linux ABI (removed from
// asm-generic/unistd.h -- newer archs are openat()-only, see
// zsyscall_linux_arm64.c's own hand-written _sysopen() shim in
// syscall_linux_arm64.h, which calls this with AT_FDCWD)
/* claude: no legacy SYS_unlink either, for the same reason there's no
 * SYS_open -- the "generic" ABI kept only the *at() forms, so remove()
 * is built on unlinkat(AT_FDCWD, path, 0) by a shim in
 * syscall_linux_arm64.h, exactly parallel to _sysopen()/openat() below.
 * chdir survived unchanged (it takes no dirfd to generalize over).
 * Both numbers read straight off scripts/syscall.tbl upstream ("35
 * common unlinkat", "49 common chdir") -- the single table arm64,
 * riscv and riscv64 all generate from, which is why their three
 * numbers_*.h files agree here too.
 */
/* claude: mkdirat, for Plan9 create()'s DMDIR bit. There is
 * deliberately no rmdir here: this ABI expresses it as
 * unlinkat(..., AT_REMOVEDIR) using the number already below, so
 * remove()'s directory case costs no new syscall on these archs at all
 * -- see syscall_linux_arm64.h's _sysrmdir(). "34 common mkdirat".
 */
/* claude: dup survived into this ABI unchanged (it takes no path to
 * generalize over), but dup2 did not -- dup3(old,new,0) replaces it,
 * and access is faccessat. See syscall_linux_arm64.h for both shims,
 * and numbers_386.h for why Plan9's dup(old,new) needs both forms.
 * "23 common dup", "24 common dup3", "48 common faccessat".
 */
#define SYS_dup	23
#define SYS_dup3	24
#define SYS_mkdirat	34
#define SYS_unlinkat	35
#define SYS_faccessat	48
#define SYS_chdir	49
#define SYS_openat	56
#define SYS_close	57
#define SYS_lseek	62
#define SYS_read	63
#define SYS_write	64
#define SYS_exit	93
/* claude: brk -- the raw kernel primitive port/sbrk.c is built on (it
 * is sbrk, not brk, that the toolchain's own callers use; see
 * include/os/mem.h and port/sbrk.c). Unlike open/unlink/mkdir/dup2/
 * access above, brk has NO *at()-style replacement and was not dropped
 * from the generic table -- it is the same call under the same name,
 * just renumbered. Note Linux's brk(2) does not use the usual
 * negative-errno convention: it returns the NEW break, and signals
 * failure by returning the UNCHANGED old one; the public Plan9-shaped
 * brk() (0/-1) is the shim in syscall_linux_arm64.h. From
 * include/uapi/asm-generic/unistd.h ("#define __NR_brk 214"), and
 * common to rv32/rv64 (not one of the 32-vs-64 split rows).
 */
#define SYS_brk	214
