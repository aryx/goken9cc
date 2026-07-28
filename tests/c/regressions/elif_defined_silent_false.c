// elif_defined_silent_false: found while porting benchs/compcert/aes.c
// to build under goken's own toolchain. aes.c's own endianness
// detection used `#if defined(X) ... #elif defined(Y) ... #else ...
// #endif`; this compiler's preprocessor doesn't support `#if` with an
// expression at all ("unknown #: if" -- see this directory's own
// hexfloat_literal.c for the same kind of preprocessor-expression
// gap, a different symptom of a related limitation). Switching the
// leading condition to `#ifdef X` (a form this preprocessor does
// support) still left a WORSE problem on the `#elif defined(Y)` line
// that follows it: that line compiles with no error or diagnostic at
// all, but its `defined(Y)` condition is silently always treated as
// false, unconditionally falling through to `#else` -- even when Y
// really is defined. A loud "unknown #: elif" (matching plain #if's
// own error) would have been far easier to catch than this.
//
// Confirmed real: BAR is #defined directly above the #ifdef/#elif
// block below, so `#elif defined(BAR)` should select `ok = 1`, not
// fall through to `#else`'s `ok = 20` -- exit(ok-1) is 19, not 0,
// before any fix.
//
// Not wired into this directory's `test:V:` (see mkfile's own
// comment): still an open bug, not a fixed one to guard. Verify by
// hand once a fix lands:
//   7c -c elif_defined_silent_false.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o elif_defined_silent_false.exe -E _main \
//      elif_defined_silent_false.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./elif_defined_silent_false.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, so this
// was verified via -H6 instead -- see docs/claude_notes/notes_libc_selfhost.txt)

extern void exit(int);

#define BAR

#ifdef FOO
int ok = 10;
#elif defined(BAR)
int ok = 1;
#else
int ok = 20;
#endif

void
main(void)
{
	exit(ok - 1);
}
