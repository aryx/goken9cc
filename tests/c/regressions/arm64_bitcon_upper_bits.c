// claude: arm64_bitcon_upper_bits: found while porting benchs/compcert/
// siphash24.c to build under goken's own toolchain -- `v2 ^= 0xff;`
// (v2 a 64-bit uvlong) silently corrupted a byte in the *upper* 32
// bits of v2 too, ones the source never touched at all.
//
// Root cause: 0xFF is a genuine, self-contained ARM64 64-bit bitmask
// immediate (8 contiguous one-bits starting at bit 0, no rotation) --
// findmask() correctly determines it needs the full 64-bit element
// size (mask->e == 64) to represent, since a smaller repeating element
// would force the upper and lower halves to match, which they don't
// (upper 32 bits legitimately all zero, lower 32 bits aren't). But
// linkers/7l/asmout.c's case 53 (immediate and/or/eor/bic encoding)
// only set the N bit (marking a genuine 64-bit-wide encoding) when the
// *raw value itself* had nonzero upper 32 bits -- true for most large
// constants, but false here despite mask->e==64 correctly saying this
// needs the 64-bit form regardless. With N wrongly left 0, the
// hardware treated it as a repeating sub-64-bit element pattern and
// replicated the low-byte value into the upper 32 bits of the
// destination register too.
//
// Confirmed real: exit(1) below, not exit(0), before any fix. Root
// cause and fix (removing the extraneous "raw value has nonzero upper
// bits" condition, trusting mask->e==64 alone) documented in
// docs/claude_notes/notes_arch_arm64.txt and benchs/compcert/mkfile's
// own siphash24.c note.
//
// Verify by hand:
//   7c -c arm64_bitcon_upper_bits.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_bitcon_upper_bits.exe -E _main \
//      arm64_bitcon_upper_bits.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_bitcon_upper_bits.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, verified
// via -H6 instead, same caveat as this directory's other arm64 repros)

typedef unsigned long long uvlong64;

extern void exit(int);

void
main(void)
{
	uvlong64 v;

	v = 0x6c7967656e657261ULL;
	v ^= 0xff;
	if (v != 0x6c7967656e65729eULL)
		exit(1);	// the bug: upper 32 bits corrupted too
	exit(0);
}
