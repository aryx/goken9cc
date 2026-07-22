/*
 * Regression test for a stack-misalignment bug in vc's align(), MIPS
 * only: the Aaut3 case ("total align of automatic") never set the
 * local `w`, so it kept the function-entry default w=1 and the final
 * xround(o, w) was a no-op -- the "safe" spill-temp region computed
 * by regsalloc() (compilers/vc/txt.c:regsalloc(), which calls
 * align(..., Aaut3)) was not rounded up to a MIPS word (SZ_LONG=4).
 * Since that region's running total feeds straight into the
 * function's frame size (see compilers/vc/sgen.c:codgen():
 * "sp->to.offset += maxargsafe;"), a single odd-sized safe temp
 * (e.g. one holding a `char`) skewed SP by a non-word amount for the
 * *whole* function, misaligning every later FP/SP-relative word
 * access in it.
 *
 * h(g()) below forces exactly that: g()'s complex (call-containing)
 * `char` argument must be evaluated into a stack temp before h can be
 * called (see compilers/vc/txt.c:garg1(), the `n->complex >= FNX`
 * case), and regsalloc() sizes that temp from the argument's own
 * (unpromoted) `char` type -- so cursafe goes 0 -> 1 without the fix.
 *
 * This was already fixed for the arm/arm64 backends when they were
 * imported from the principia-softwarica lineage (see
 * compilers/5c/swt.c and compilers/7c/swt.c: "w = SZ_LONG; /* because
 * of a pun in cc/dcl.c:contig() * /"), just never ported to vc
 * (mips). 9front independently found and fixed the same bug in their
 * vc: commit e8c8f5d82 "vc: word align automatics".
 *
 * There's no reliable way to observe the resulting misalignment at
 * *runtime* here: on Linux, an unaligned MIPS word access is normally
 * fixed up transparently by the kernel trap handler instead of
 * crashing, so a wrong build could easily still "pass" a run-and-diff
 * test by accident. Instead this greps vc's own -A alignment trace
 * (see compilers/vc/swt.c:align(): "align %s %ld %T = %ld") and
 * checks the "aut3" result is a multiple of 4 -- see mkfile.
 */
char g(void) { return 1; }
void h(char a) { }

int
main(void)
{
	h(g());
	return 0;
}
