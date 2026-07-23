// riscv64_ptr_width: found while bringing up jc (the riscv64 C compiler,
// thechar='j' -- see compilers/ic/mkfile, compilers/ic/txt.c ginit()).
//
// src/cmd/cc/lex.c's cinit() builds the canonical pointer type
// (types[TIND] = typ(TIND, types[TVOID])) *before* ic's ginit() runs (main()
// calls tinit()/cinit() before ginit()). typ() bakes in width = ewidth[TIND]
// at that exact call time -- which for riscv is gc.h's compile-time SZ_IND
// (4, the rv32/'i' default), since ewidth[TIND] is only overridden to 8 for
// 'j' inside ginit(), which hasn't run yet. Every pointer type parsed from
// source afterwards (e.g. "char *x") gets a *fresh* typ() call and so gets
// the right width automatically -- but this one pre-existing object stays
// stale at width=4 unless patched directly (now done in ginit()).
//
// The practical effect: a string-literal argument's type reused this stale
// object, so the caller's stack-argument layout (compilers/ic/swt.c align(),
// case Aarg2: "o += t->width") reserved only 4 bytes for it instead of 8,
// shifting every later stack argument in the same call by 4 bytes relative
// to where the callee's own (freshly, correctly, 8-byte-wide-typed)
// prologue expects to find them.
//
// This function's own "s" parameter is unaffected (its type is fresh,
// parsed after ginit()); it's specifically a string-literal *argument at a
// call site* that triggers it, hence passing "x" below rather than a local
// char* variable.

extern void exit(int);

static void
check(char *s, int v)
{
	if(v != 77)
		exit(1);
}

void
main(void)
{
	check("x", 77);
	exit(0);
}
