// Regression test for compilers/6c/cgen.c's immconst() using `long v`
// instead of `int32 v`: on our 64-bit hosts `long` is 64 bits, so
// `v = n->vconst; n->vconst == (vlong)v` never differs and immconst()
// always claimed a constant "fits as a 32-bit signed immediate", even
// for vlong constants that don't. hardconst() (which callers use to
// decide whether a constant needs to be materialized into a register
// first) is defined as `!immconst(n)`, so it never fired either.
//
// The result: comparing a vlong against an out-of-range constant
// generated an unencodable instruction, e.g. "CMPQ AX,$1099511627818"
// -- x86-64 has no 64-bit-immediate CMP form, only a 32-bit
// sign-extended one, so this isn't just wrong codegen but an assembly
// failure ("doasm: notfound"). Found while switching tests/c/mini2's
// regress7c_macos_amd64.exe off 6cg (that file's id64(1099511627818LL)
// == 1099511627818LL comparison, 1099511627818 = 2^40+42, is the exact
// same pattern, just only exercised there under -H6/macOS). Go-era's
// own 6c (src/cmd/6c/cgen.c immconst()) already uses int32 here.

typedef long long vlong;

extern int write(int fd, char *buf, int count);
extern void exit(int code);

vlong
id64(vlong v)
{
	vlong *p;

	// taking &v forces v to live in memory, matching the original
	// repro (regress7c.c) exactly
	p = &v;
	return *p;
}

void
main(void)
{
	if(id64(1099511627818LL) == 1099511627818LL)
		write(1, "PASS\n", 5);
	else
		write(1, "FAIL\n", 5);
	exit(0);
}
