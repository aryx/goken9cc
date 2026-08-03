// claude: arm64_large_bss_sb_offset: found while tracking down why
// benchs/compcert/nsieve.c, nsievebits.c and spectral.c started
// crashing (SIGSEGV) on arm64 only, right after main() gained real
// argc/argv (commit 49e29b025, "wire real argc/argv into every arch's
// rt0.s") -- the argc/argv change itself turned out to be a red
// herring; it only mattered because it was the first time main() also
// called atoi(), and *that* was what perturbed 7l's dodata() hash-
// table iteration order enough to place some small global (fmt.c's
// static `fmtalloc` table) after lib_core/libc/port/minimal_malloc.c's
// 64MB `heap[]` array in the linked data/bss segment -- pure ordering
// luck, unrelated to argc/argv at all.
//
// Root cause: linkers/7l/asmout.c's case 30/31 (global-variable
// load/store through a "L(R)" SB-relative operand) materializes the
// target address as REGSB + hi via a single ADD-immediate instruction
// (oaddi()), which can only encode ~16MB (imm12, optionally <<12).
// Once a symbol's *linked* SB-offset exceeds that -- trivially true
// for any access indexing more than ~16MB into a single large global,
// regardless of what else got linked in or in what order -- the ADD
// silently truncated the high bits (oaddi()'s own overflow check only
// tested bits [23:12], missing values with nothing set there but bits
// set at 24+, e.g. a plain 0x4000000/64MB offset) instead of erroring,
// producing a validly-encoded but wrong address. 9front's 7l already
// has the fix for this (case 30/31's `if(v<0 || (v>>s)>=(1<<24)) goto
// Hugestxr/Hugeldxr;`, plus a working olsxrr() and case 47/48 using
// it) -- goken's port had case 47/48 already written but dead (never
// reached, and itself buggy: omovlit() called with AMOVW instead of
// AMOV, so it only materialized the low 32 bits of REGTMP) and
// olsxrr() was still a stub. See docs/claude_notes/notes_arch_arm64.txt
// for the full writeup and how the crash was actually diagnosed
// (dodata() instrumentation + qemu-gdb register/memory inspection).
//
// Deliberately indexes a *compile-time-constant* huge offset into a
// single global array rather than relying on the linker happening to
// place some other small symbol far away -- that made the original bug
// depend on hash-table iteration order (see above), which isn't a
// reliable regression gate. Indexing directly makes the total SB-
// offset (array's own linked offset + this constant index) exceed the
// ~16MB threshold unconditionally, regardless of anything else linked
// alongside this file.
//
// Writes through `p` (an address-of computation, case 65's already-
// correct codegen -- entirely different from case 30/31's buggy L(R)
// path) rather than through `big[N-1]` directly: the write and the
// read of a *plain* `big[N-1] = X; if(big[N-1] != X)` both go through
// the same buggy oaddi()-based address computation, so they'd
// consistently agree on the same *wrong* address and this test would
// pass even with the bug present (confirmed empirically -- the naive
// version of this test didn't actually catch the regression). Writing
// through the independently-correct `p` and reading through the
// case-31 path being tested means a wrong address in the read shows up
// as a real mismatch instead of two wrongs silently agreeing.
//
// Verify by hand:
//   7c -c arm64_large_bss_sb_offset.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_large_bss_sb_offset.exe -E _main \
//      arm64_large_bss_sb_offset.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_large_bss_sb_offset.exe; echo $?
// (want 0)

extern void exit(int);

enum { N = 20*1024*1024 };	/* > 16MB: past a single ADD-immediate's reach */

char big[N];
char *p;

void
main(void)
{
	p = &big[N-1];		/* address-of: a different, already-correct codegen path */
	*p = 33;
	big[0] = 11;
	if(big[0] != 11)
		exit(1);	/* our own write got clobbered */
	if(big[N-1] != 33)	/* the case-31 path under test */
		exit(2);
	exit(0);
}
