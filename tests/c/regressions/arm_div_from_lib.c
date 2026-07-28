// arm_div_from_lib: found while building lib_core/libc's arm libc.a and
// linking a real C program (using integer division, inside fmt/'s
// __ifmt) against it via -lc -- not diff-derived, a goken-original
// linker bug.
//
// arm (5c/5a/5l) has no hardware integer divide, so 5c emits ADIV/
// AMOD pseudo-instructions instead of a real divide, and 5l's own
// noops() (linkers/5l/noop.c) rewrites those into real calls to
// runtime helpers (_div, _divu, _mod, _modu) via initdiv(). The bug:
// initdiv() looks for those helpers' TEXT definitions only in the
// program text *already loaded* (the firstp linked list) -- but
// noops() runs strictly *after* loadlib()'s one-shot library-scanning
// pass (see linkers/5l/main.c's loadlib()/patch()/dodata()/follow()/
// noops() ordering), so if _div's definition lives inside a *library*
// (-lXXX) rather than being directly named on the link command line,
// loadlib() never had a reason to pull it in yet -- initdiv() finds
// nothing and diags "undefined: _div" (and _divu/_mod/_modu) instead
// of ever re-consulting the library.
//
// Every prior arm test in this tree (tests/c/mini2, tests/c/vlong)
// sidestepped this entirely by linking _div's definition in *directly*
// (as one of the .5 files named on the 5l command line, e.g.
// tests/c/mini2/linux_arm.s's own div.s-derived block), which loadlib()
// doesn't need at all -- this is the first time _div lives inside an
// actual archive.
//
// Fixed by having initdiv() mark any still-missing div/mod symbol
// SXREF and call loadlib() again (mirroring what the `dlm` branch a
// few lines above already does via sdiv()) before giving up, so a
// definition newly available in a library actually gets pulled in.
//
// Confirmed real before the fix: linking against a library containing
// only _div/_divu/_mod/_modu (not named directly on the 5l command
// line) failed with "undefined: _div" and friends; after the fix, the
// runtime division results below all check out.

extern void exit(int);

int
main(void)
{
	int a, b;

	a = 100;
	b = 7;
	if(a/b != 14)
		exit(1);
	if(a%b != 2)
		exit(2);

	a = -100;
	if(a/b != -14)
		exit(3);
	if(a%b != -2)
		exit(4);

	exit(0);
	return 0;
}
