// claude: arm64_switch_vlong_wide_range: found writing
// arm64_switch_vlong.c's own regression test for a DIFFERENT bug
// (typeswitch, see that file) -- adding a third case value next to
// that file's own 0 and 2^33 (specifically 2^33+1, one more than the
// existing 2^33 case) made 7c hang indefinitely instead of just
// mis-compiling, so this got split into its own dedicated test rather
// than folded into arm64_switch_vlong.c, to keep each test isolated
// to the one bug it's meant to catch.
//
// Two real, separate bugs in compilers/7c/swt.c's swit2(), both found
// getting this one repro to actually pass:
//
// 1. The hang: swit2() decides whether a case set is "dense enough"
//    for a direct jump table via `i = (q+nc-1)->val - (q+0)->val; if
//    (i > 0 && i < nc*2) goto direct;` -- but `i` was a plain `int`,
//    truncating the real ~8.6-billion-wide spread between 0 and
//    2^33+1 down to 1, which (being "< nc*2") wrongly took the
//    `direct` path meant only for a tightly clustered case set. Once
//    there, `v` (also meant to walk the full case-value range one
//    step at a time) was `int32`, permanently unable to reach a
//    `q->val` comparison target of 2^33+1 -- an infinite loop, not
//    just a wrong table, confirmed by reproducing it standalone (7c
//    hangs, not crashes, on exactly this switch shape). Fixed by
//    widening both to a real vlong (a fresh `range` variable for the
//    first, rather than reusing `i`, which this function's OTHER
//    loops already use as a plain 0..nc-1 counter).
// 2. Once the hang was fixed and the range check correctly rejected
//    this case set for the `direct` path, the FALLBACK path (a plain
//    per-case compare chain, swit2()'s own `nc<5` branch) still
//    produced wrong answers: it builds each case's comparison via
//    `nodconst(q->val)`, and nodconst() takes a plain int32,
//    silently truncating any case value that doesn't fit. Fixed by
//    using nodgconst(v, n->type) instead (already existed, just
//    unused outside txt.c and undeclared in gc.h) -- it dispatches to
//    a real 64-bit constant node when the switch's own type is
//    vlong-shaped, falling back to nodconst's plain int32 path
//    otherwise, so it's correct for a 32-bit switch too.
//
// Confirmed against 9front's own compilers/7c/swt.c: it has the
// identical `int i` truncation (bug 1's first half). Its own `v` is a
// real `long`, 64-bit on Plan9's own arm64 host, so it doesn't share
// bug 1's second half; not checked whether 9front's own nodconst()
// has the same int32-only shape as bug 2 -- not confirmed hit
// upstream either way, not blindly matched here. See
// docs/claude_notes/notes_arch_arm64.txt.
//
// Verify by hand:
//   7c -c arm64_switch_vlong_wide_range.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_switch_vlong_wide_range.exe -E _main \
//      arm64_switch_vlong_wide_range.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_switch_vlong_wide_range.exe; echo $?
// (want 0) -- also worth just timing this compile by hand: the
// pre-fix version hangs 7c indefinitely rather than erroring, so a
// regression here would hang `mk test`, not fail it cleanly.

extern void exit(int);

long long
classify(long long v)
{
	switch(v){
	case 0:
		return 100;
	case 8589934592LL:		/* 2^33 */
		return 200;
	case 8589934593LL:		/* 2^33 + 1 -- adjacent to the case above */
		return 300;
	default:
		return -1;
	}
}

void
main(void)
{
	if(classify(0) != 100)
		exit(1);
	if(classify(8589934592LL) != 200)
		exit(2);
	if(classify(8589934593LL) != 300)
		exit(3);
	if(classify(1) != -1)
		exit(4);
	exit(0);
}
