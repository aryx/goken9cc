// mulzero: found via diff analysis of the amd64 toolchain (6a/6c/6l)
// against 9front (scripts/diff.rc, ~/xxx/9front/compilers/6c/mul.c).
// Same bug, independently, in compilers/8c/mul.c and compilers/8ck/mul.c.
//
// mulgen1() (the constant-multiply strength-reduction search) never
// special-cased a multiplier of 0: mulparam(0, p) could find a
// spurious "algorithm" whose arg didn't match any case mulgen1()'s own
// switch handles, falling through to "goto bad; diag(Z, \"mulgen
// botch\");" -- so "x *= 0;" failed to compile outright with an
// internal-error diagnostic (and, before erroring, had already emitted
// code that just left x unchanged instead of computing 0).
//
// Fixed by adding the same 3-line early-out 9front has, right at the
// top of mulgen1(): "if(v == 0){ zeroregm(n); return 1; }" (zeroregm()
// already existed, used elsewhere for the same "materialize a zero"
// purpose).
//
// Confirmed real before landing the fix: "6c -c mulzero.c" exited 1
// with "mulzero.c:N mulgen botch" on all three of 6c/8c/8ck.

extern void exit(int);

void
main(void)
{
	int x;

	x = 5;
	x *= 0;
	exit(x);
}
