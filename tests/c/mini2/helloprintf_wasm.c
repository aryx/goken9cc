// A working end-to-end test, wired into this directory's mkfile/
// test_wasm: `ec -o helloprintf_wasm.e helloprintf_wasm.c` compiles
// this file, `ec -o print_nofloat_no64.e print_nofloat_no64.c`
// compiles the shared print_nofloat_no64.c unmodified, and linking
// both together with wasi_wasm.s via el now produces a module whose
// stdout matches helloprintf_wasm.expected.txt byte for byte -- see
// docs/notes_wasm.txt's "Open questions" section (the two entries this
// resolved: ec's gextern() emitting real ADATA for initialized global/
// static data, and el scoping D_STATIC symbols per input file instead
// of merging same-named statics like the `.string` blob across files)
// for how each of the two gaps that used to block this got fixed.
//
// This covers what real printf() needs beyond classify_switch()/
// loopswitch() (regress_wasm.c): %t/%x/%s/%d, multiple "..." arguments,
// the variadic-call ABI's `arg = (byte*)(&s+1)` address-of-a-named-param
// trick inside printf() itself, and now print_nofloat_no64.c's own
// initialized statics (`fd`, `dig`). Not the full helloprintf.c: that
// also exercises fact()/fact_iter() (fine) but test()/test_hello()
// (test.c uses uint64/float64, and exit() has no wasi wiring yet --
// both still out of scope here).
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
