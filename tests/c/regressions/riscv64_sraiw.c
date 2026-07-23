// riscv64_sraiw: found empirically while re-checking the todo.org riscv64
// candidate list now that jc/ja/jl actually work. linkers/il/optab.c's
// slliw/srliw/sraiw (the RV64-only word-shift-immediate ops, used for
// shifts on plain 32-bit ints) had "type" field 2, routing them through
// asmout()'s case 2 ("addi $I,[R,]D" -- a plain 12-bit signed immediate,
// no shift-type bit). The sibling 64-bit-register shift-immediate ops
// (slli/srli/srai) use case 1 instead, which additionally does
// "v |= (o->param<<5)" to OR the arithmetic-vs-logical distinguishing
// bit (o->param is 0x20 for the arithmetic variants) into the immediate
// field. Since case 2 never applies o->param at all, SRAIW's encoding
// came out byte-identical to SRLIW's -- it silently executed as a
// *logical* right shift instead of arithmetic, discarding the sign.
// SLLIW/SRLIW happened to still work (they don't need that bit: SLL's
// param is 0, and func3 alone already distinguishes SLL/SRL from ADD),
// which is why this stayed hidden even after jc started passing every
// other test.
//
// Fix: give slliw/srliw/sraiw the same "type 1" case 1 already correctly
// used by slli/srli/srai (matches the fix in the miller-riscv fork,
// found via scripts/diff.rc's ~/xxx/miller-riscv/il/optab.c comparison).
//
// Confirmed real: before the fix, "x = -8; x >>= 2;" (below) computed a
// large positive number under qemu-riscv64 instead of -2.

extern void exit(int);

void
main(void)
{
	int x;

	x = 1;
	x <<= 3;
	if(x != 8)
		exit(1);

	x = 17;
	x >>= 2;	// unsigned/logical would also give 4 here -- not discriminating
	if(x != 4)
		exit(2);

	x = -8;
	x >>= 2;	// arithmetic: -2. logical (the bug): a large positive number.
	if(x != -2)
		exit(3);

	exit(0);
}
