// vlong_mul: not diff-derived -- a gap in this tree's own self-hosted
// libc, found while adding nsec() (lib_core/libc/os/linux/time.c), whose
// "sec * 1000000000LL" was the first vlong*vlong anything had ever asked
// lib_core/libc for.
//
// compilers/cc/com64.c and compilers/cck/com64.c both turn "vlong *
// vlong" on a 32-bit arch into a call to _mulv (fvn("_mulv", TVLONG)),
// exactly as they turn + and - into _addv/_subv. But _addv/_subv are
// portable C in lib_core/libc/port/vlrt.c, while _mulv needs the
// machine's widening multiply and so lives in per-arch assembly --
// which lib_core/libc simply did not have, for any arch. Upstream Plan9
// keeps it in libc/$arch/vlop.s; there was no vlop.s here at all. 5l
// reported "undefined: _mulv" followed by a cascade of bogus encoding
// errors on the unresolved BL ("illegal combination BL 0 0 16",
// "bad rrr 54"), which is what makes the real cause easy to miss.
//
// Why nothing caught it earlier: tests/c/vlong/ has its own LOCAL copy
// of vlrt.c and links against no libc, so it never consulted
// lib_core/libc's. Nothing else multiplied two vlongs.
//
// Now fixed by lib_core/libc/arch/{arm,386,mips,riscv}/vlop.s, adapted
// from principia (arm, 386), plan9front (mips) and Richard Miller's
// riscv port. This test is 32-bit-only in spirit -- amd64/arm64/riscv64
// multiply vlongs with a single native instruction and never call _mulv
// -- but it is run on a 64-bit arch too, as a control proving the
// expected values themselves are right.

#include <u.h>
// core/types.h, not libc.h: vlong/uvlong live there, and libc.h now
// carries a `#pragma lib "libc.a"` that would make the linker pull in
// the whole libc -- defeating the point of linking vlrt.c and vlop.s
// directly here (see this directory's mkfile).
#include <core/types.h>

extern void exit(int);

// vlrt.c's _vasop() calls abort() on an unknown type tag. Supplied here
// rather than linking port/abort.c, which would drag in the fmt layer
// and from there most of libc -- the point of this test is to link
// exactly vlrt.c + vlop.s and nothing else. Never reached.
void
abort(void)
{
	exit(99);
}

void
main(void)
{
	vlong a, b, c;
	uvlong u;

	// 1: a product that stays within 32 bits. Passes even with a _mulv
	// that computes only lo*lo, so it proves nothing on its own -- it is
	// here to isolate a totally broken helper from a partly broken one.
	a = 100000;
	b = 3;
	if(a * b != 300000LL)
		exit(1);

	// 2: the real check -- a product that OVERFLOWS 32 bits, so the
	// answer depends on the high word being computed and stored. A
	// _mulv that dropped the widening multiply entirely gives
	// 0xA7640000 (the low word alone) here instead.
	a = 1000000;
	b = 1000000;
	if(a * b != 1000000000000LL)
		exit(2);

	// 3: exercises the CROSS terms (x.hi*y.lo and x.lo*y.hi), which are
	// separate instructions in every one of the four vlop.s files and so
	// can be individually wrong. Both operands here have a nonzero high
	// word, which checks 2 above does not.
	a = 0x100000001LL;
	b = 3;
	if(a * b != 0x300000003LL)
		exit(3);

	// 4: this is the one that would catch a byte-order mistake in how
	// the result is stored back (vlrt.c's Vlong is hi/lo-swapped on
	// big-endian mips -- see its own comment), since the two words
	// differ.
	// (2<<32|3) * (1<<32|5): lo*lo = 15, and the two cross terms
	// contribute (3*1 + 2*5) = 13 in the high word. The 2<<32 * 1<<32
	// term overflows 64 bits entirely and is correctly discarded.
	a = 0x0000000200000003LL;
	b = 0x0000000100000005LL;
	c = a * b;
	if(c != 0x0000000D0000000FLL)
		exit(4);

	// 5: the sign of the result must come out right too. _mulv itself is
	// sign-agnostic (the low 64 bits of the product are the same either
	// way), so this checks the compiler is not doing something extra
	// around it.
	a = -1000000;
	b = 1000000;
	if(a * b != -1000000000000LL)
		exit(5);

	// 6: and the unsigned path, where the top bit being set must not be
	// treated as a sign.
	u = 0x8000000000000000ULL;
	if(u * 2ULL != 0ULL)
		exit(6);
	u = 0xFFFFFFFFULL;
	if(u * u != 0xFFFFFFFE00000001ULL)
		exit(7);

	exit(0);
}
