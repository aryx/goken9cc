// arm64_uvlong_shift32: found while porting benchs/compcert/sha3.c and
// siphash24.c to build under goken's own toolchain -- both compute
// wrong hashes despite building and running cleanly (no crash), which
// ruled out every previously-found bug this session (all of those
// either failed to compile/link, or crashed outright).
//
// Two independent bugs bundled here, both now fixed (see
// docs/claude_notes/notes_arch_arm64.txt for the full writeups):
//
// - x != 0x0123456789ABCDEFULL: compilers/7c/reg.c's mkvar() forced
//   any bare (symbol-less) constant destined for registerization to
//   `et = TLONG` (4 bytes) unconditionally, and its own `o` (the
//   constant's value) was declared `int32`, truncating it on the very
//   next line regardless. Every consumer of the resulting Var's
//   ->etype (e.g. addmove(), picking AMOV vs AMOVW when reloading a
//   registerized constant) trusted that wrongly-narrow width, so any
//   64-bit-valued constant used more than once got its high 32 bits
//   silently dropped on reload. Fixed: `o` is now `vlong`, and `et`
//   only defaults to TLONG when the value actually round-trips through
//   a 32-bit sign extension.
//
// - s32 != 0x100000000ULL (1ULL<<32, or any other 64-bit-valued
//   constant that isn't the direct operand of a plain assignment --
//   e.g. compared, added to, or otherwise consumed by a non-MOV
//   instruction): linkers/7l/span.c's addpool() only ever widened a
//   literal-pool entry to 8 bytes (ADWORD) when the *instruction*
//   using it was itself a MOV; for any other instruction (CMP here)
//   needing the pool, the constant got a 4-byte (AWORD) entry no
//   matter its actual value, truncating it to its low 32 bits (0 for
//   this specific value) before the comparison ever runs. Fixed by
//   also widening whenever the constant doesn't already fit as its own
//   zero-extended low 32 bits, porting the relevant condition from
//   9front's 1d330c0bd (not a verbatim sync -- see the notes file).
//
// sha3.c/siphash24.c themselves are *not* unblocked by this alone:
// both still fail every test vector after these two fixes, so there is
// at least one more, distinct, not-yet-isolated bug affecting the
// specific way they compute (basic 64-bit shift/rotate/add/xor with
// large constants, global array initializers, and register-count
// shifts up to 63 were all individually re-verified correct after
// these fixes and are NOT the remaining cause) -- see benchs/compcert/
// mkfile's own "still blocked" note.
//
// Verify by hand:
//   7c -c arm64_uvlong_shift32.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_uvlong_shift32.exe -E _main \
//      arm64_uvlong_shift32.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_uvlong_shift32.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, so this
// was verified via -H6 instead, using a padded, print()-based scratch
// copy to work around -H6's own separate small-binary truncation bug
// -- see notes_libc_selfhost.txt -- rather than this checked-in,
// deliberately minimal file, which is too small to link past that bug
// on -H6 as-is)

typedef unsigned long long uvlong64;

extern void exit(int);

void
main(void)
{
	uvlong64 x, s4, s32, s33;

	x = 0x0123456789ABCDEFULL;
	if(x != 0x0123456789ABCDEFULL)
		exit(1);	// a plain 64-bit constant already fails to compare equal to itself

	s4 = 1ULL << 4;
	if(s4 != 16ULL)
		exit(2);	// shift by a small amount: expected to work, sanity check

	s32 = 1ULL << 32;
	if(s32 != 0x100000000ULL)
		exit(3);	// shift by >= 32: the actual bug

	s33 = 1ULL << 33;
	if(s33 != 0x200000000ULL)
		exit(4);

	exit(0);
}
