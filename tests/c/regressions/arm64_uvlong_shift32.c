// arm64_uvlong_shift32: found while porting benchs/compcert/sha3.c and
// siphash24.c to build under goken's own toolchain -- both compute
// wrong hashes despite building and running cleanly (no crash), which
// ruled out every previously-found bug this session (all of those
// either failed to compile/link, or crashed outright).
//
// Root-caused with a series of isolated repros (this file is the
// distilled result) run natively on darwin/arm64: a plain 64-bit
// (`unsigned long long`, this compiler's `uvlong`) constant already
// fails a load-and-compare (`x == 0x0123456789ABCDEFULL` evaluates
// false), and left/right shifts by an amount >= 32 compute the wrong
// result -- shift by 4 is correct, shift by 32 or 33 isn't. Not yet
// root-caused into 7c's own codegen (compilers/7c/cgen.c or txt.c);
// this file only isolates and pins down the symptom.
//
// Confirmed real: exit(1)/(2)/(3) below, not exit(0), before any fix.
// No libc/print dependency -- pure computation, checked via exit code,
// same shape as this directory's other regressions.
//
// Not wired into this directory's `test:V:` (see mkfile's own
// comment): still an open bug, not a fixed one to guard. Verify by
// hand once a fix lands:
//   7c -c arm64_uvlong_shift32.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_uvlong_shift32.exe -E _main \
//      arm64_uvlong_shift32.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_uvlong_shift32.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, so this
// was verified via -H6 instead, using padded, print()-based scratch
// copies to work around -H6's own separate small-binary truncation
// bug -- see notes_libc_selfhost.txt -- rather than this checked-in,
// deliberately minimal file, which is too small to link past that
// bug on -H6 as-is)

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
