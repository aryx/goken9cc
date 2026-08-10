// A real, running tests/c/mini2 example for wasm, in the same spirit
// as helloprintf.c for every other arch: a C program compiled by ec,
// linked by el, and its actual stdout compared against expected.txt --
// not the self-checking return-code trick regress_wasm.c uses (ec has
// no printf/varargs yet, see below).
//
// Not helloprintf.c itself: real printf needs its own `arg =
// (byte*)(&s+1)` to find its variadic arguments, which takes the
// address of a parameter -- something ec can't do yet (see
// docs/notes_wasm.txt's "Open questions": the address-taken-local/
// shadow-stack split). write() takes no address of anything on the
// caller's side (wasi_wasm.s's own wrapper builds its iovec out of a
// private global, never the caller's frame), so it's what's reachable
// today: string literals (ec/swt.c's outstring()) and taking the
// address of a *global*/*static* (ec/cgen.c's OADDR case) -- both new
// this session -- are enough for this, not for varargs.
// claude: void, not int -- must match minilibc.h's own `extern void
// write(...)` prototype (what print_nofloat_no64.c's vprintf() compiles
// against) and wasi_wasm.s's own real, void SIGNATURE for write() (see
// its comment): ec's cgen() decides whether a bare call-statement needs
// a wasm DROP purely from the *caller's* declared C return type, so a
// mismatched prototype here would silently desync this file's own
// stack bookkeeping from write()'s real (void) wasm signature.
extern void write(int fd, char *buf, int n);

void
main(void)
{
	write(1, "Hello, wasm!\n", 13);
}
