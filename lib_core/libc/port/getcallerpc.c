/*s: libc/port/getcallerpc.c */
/*s: libc includes */
#include    <u.h>
#include    <libc.h>
/*e: libc includes */
/* claude: this whole file is #ifdef-guarded out for 386/amd64/arm,
 * which get a real implementation instead (lib_core/libc/arch/
 * {386,amd64,arm}/getcallerpc.s) -- compiled unconditionally for every
 * arch anyway (same "empty object file where irrelevant" pattern as
 * e.g. os/linux/open.c's own #ifdef mips), so it's always exactly one
 * or the other, never both (which would be a duplicate-symbol link
 * error) and never neither (which would leave getcallerpc()
 * undefined). Nested #ifdef, not `#if !defined(X) && ...` -- 7c/vc/ic's
 * preprocessor doesn't understand `#if` with an expression at all (see
 * port/seek.c's identical comment). 386 is checked via `cputype386`
 * (mkfiles/386/mkfile's own -D, not lib_core/libc/mkfile's usual bare
 * -D$cputype): a purely-numeric #ifdef operand like plain `386` fails
 * outright ("syntax in #if(n)def") -- see that mkfile's own comment.
 *
 * arm64/mips/riscv/riscv64 (the archs that actually reach the body
 * below) have neither a principia-softwarica reference to adapt (that
 * project predates all four) nor a from-scratch implementation
 * attempted here -- adapted instead from principia's OWN admitted
 * fallback for archs it has no per-arch version for either
 * (~/principia-softwarica/lib_core/libc/port/getcallerpc.c, itself
 * just `return 0;`). Safe because nothing in this codebase actually
 * reads the value yet (only used for setmalloctag()'s own no-op debug
 * bookkeeping, port/minimal_malloc.c) -- implement for real, per-arch,
 * before anything starts relying on it for more than that.
 */
/*s: function [[getcallerpc]] */
#ifdef cputype386
#else
#ifdef amd64
#else
#ifdef arm
#else
uintptr
getcallerpc(void *v)
{
    USED(v);
    return 0;
}
#endif
#endif
#endif
/*e: function [[getcallerpc]] */
/*e: libc/port/getcallerpc.c */
