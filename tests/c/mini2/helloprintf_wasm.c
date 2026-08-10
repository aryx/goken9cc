// NOT YET a working end-to-end test (not wired into this directory's
// mkfile/test_wasm on purpose) -- kept as a checked-in record of how
// far real printf() reaches today and exactly what still blocks it.
// `ec -o helloprintf_wasm.e helloprintf_wasm.c` compiles this file
// cleanly on its own, and -- more importantly -- `ec -o x.e
// print_nofloat_no64.c` (unmodified) now *also* compiles cleanly:
// until reg.c's hoistswitches() (docs/notes_wasm.txt's "Switch/case"
// section), vprintf()'s own `for(...) { ...continue... switch(){}
// switch(){} }` shape was a real, open relooper-class limitation.
//
// Linking this file together with print_nofloat_no64.c and
// wasi_wasm.s currently produces a module that validates and runs but
// prints nothing, because of two *separate*, still-open, pre-existing
// gaps (see notes_wasm.txt's "Open questions" for both): ec doesn't
// implement initialized global/static data yet (print_nofloat_no64.c's
// `static int32 fd = 1;` silently comes out as 0, so its own
// write(fd,...) calls target fd 0/stdin instead of 1/stdout), and el
// merges two input files' own same-named file-local (CSTATIC) symbols
// -- both this file's and print_nofloat_no64.c's own `.string` blob --
// into one, corrupting both files' string-literal data. Neither is a
// switch/control-flow issue; both need their own separate fix.
//
// Once both are fixed, this covers what real printf() needs beyond
// classify_switch()/loopswitch() (regress_wasm.c): %t/%x/%s/%d,
// multiple "..." arguments, and the variadic-call ABI's `arg =
// (byte*)(&s+1)` address-of-a-named-param trick inside printf() itself.
// Not the full helloprintf.c: that also exercises fact()/fact_iter()
// (fine) but test()/test_hello() (test.c uses uint64/float64, and
// exit() has no wasi wiring yet -- both out of scope here too).
#include "minilibc.h"

// claude: print_nofloat_no64.c's '!' case (an unreachable format
// specifier in this test) still calls panic() -- wasm requires every
// referenced function to resolve at link time even if never actually
// invoked at runtime (el does no dead-code elimination), so this needs
// a real, if unused, definition.
void
panic(int x)
{
}

void
main(void)
{
	printf("Hello World: %t\n", true);
	printf("Hello World: %x\n", 42);
	printf("Hello %s%s: %d\n", "Wor", "ld", 42);
	printf("Hello World: %d\n", 42);
}
