// Minimal repros for two 7c code generation bugs found while porting
// the tests to macOS arm64 (July 2026, hello_macos_arm64 branch), kept
// as a runtime regression test. Expected output is two PASS lines.
// The bugs were OS-independent; they just happened to be exposed (and
// natively debuggable) on macOS. See docs/notes_macos.txt for the
// debugging session that found them.

#pragma lib "libprint.a"
#include "minilibc.h"

// ------------------------------------------------------------------
// Bug 1: 7c/sgen.c gtext() overwrote the frame size (stkoff) set by
// gpseudo(ATEXT, ...) with 0, so the frame only ended up covering
// maxargsafe (the outgoing-argument build area added by cc/pgen.c).
// The locals, laid out by cc at the TOP of the frame (autosize-8,
// autosize-16, ...), then landed on the same stack slots as the
// outgoing arguments of any call with 3+ arguments (slots 16(R31),
// 24(R31), ...), so a stack local was silently corrupted by making a
// call. In tests/c/mini2 this corrupted vprintf's spilled 'p' pointer
// on every write() call.
// ------------------------------------------------------------------

int
f3(int a, int b, int c)
{
	return a + b + c;
}

int
frame_bug(void)
{
	// an array so that it lives on the stack (a scalar local would
	// be registerized and dodge the bug)
	int buf[4];

	buf[0] = 39;
	buf[1] = 38;
	// 3 arguments: the 3rd goes to stack slot 24(R31), which with
	// the bug was also buf[0]'s home (autosize-16)
	f3(1, 2, 3);
	return buf[0] + 3;	// 42, or 6 (= 3+3) with the bug
}

// ------------------------------------------------------------------
// Bug 2: the caller (7c/txt.c garg1) decides with the typechlpv table
// whether the FIRST argument is passed in the register REGARG (R0):
// on arm64 that includes vlong. But the callee prologue code
// (cc/pgen.c "isolate first argument") used the typechlp table (no
// vlong) to decide whether to spill R0 back to the 0(FP) stack slot.
// So a vlong first argument arrived in R0 but the callee read
// uninitialized stack when accessing it from memory. Fixed like in
// 9front by using the per-compiler typeword table on the callee side.
// In tests/c/vlong this made printint(int64) print stack garbage.
// ------------------------------------------------------------------

int64
id64(int64 v)
{
	int64 *p;

	// taking &v forces v to live in memory in the callee, so the
	// callee must spill R0 to 0(FP) in its prologue: exactly the
	// spill that was missing
	p = &v;
	return *p;
}

void
main(void)
{
	if(frame_bug() == 42)
		printf("PASS: locals do not overlap outgoing args\n");
	else
		printf("FAIL: gtext frame size bug is back (7c/sgen.c)\n");

	if(id64(1099511627818LL) == 1099511627818LL)	// 2^40 + 42
		printf("PASS: vlong first arg spilled by callee\n");
	else
		printf("FAIL: vlong REGARG bug is back (cc/pgen.c typeword)\n");

	exit(0);
}
