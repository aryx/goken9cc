// claude: hex_const_no_suffix_truncated: found while porting benchs/
// compcert/sha3.c to build under goken's own toolchain -- a
// `const uint64[24]` table of round constants, written (as in the
// reference Keccak implementation) as plain hex literals with no U/L/
// ULL suffix, silently had every entry needing more than 32 bits
// zeroed in its high half. Root cause is in the lexer's numeric-
// constant classifier: an unsuffixed literal only ever gets tried
// against TINT then TUINT (32-bit) -- there is no further fallback to
// TVLONG/TUVLONG when the value doesn't fit either. Real C89/C99 (and
// gcc/clang, which is why the sha3.c bug never showed up in the
// existing gcc/clang comparison) instead keep promoting through long
// and long long as needed, never silently truncating. A `-w`-only
// "truncated constant" warning does fire, but is silent by default --
// this looks like clean, working code without it. See
// docs/claude_notes/notes_shared_frontend_bugs.txt for the full
// writeup, including confirmation this is present identically in all
// three frontend forks in this tree (src/cmd/cc, compilers/cck,
// compilers/cc2), not just the one this file happens to test.
//
// Confirmed real: exit(1) below, not exit(0), before any fix.
//
// Not wired into this directory's `test:V:` (see mkfile's own
// comment): still an open bug, not a fixed one to guard. Verify by
// hand once a fix lands (shown here for 7c; the bug is not
// arm64-specific -- see the lexer files listed above for which
// frontend a given backend actually builds against):
//   7c -w -c hex_const_no_suffix_truncated.c   # expect no "truncated
//                                               # constant" warning
//   7c -c hex_const_no_suffix_truncated.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o hex_const_no_suffix_truncated.exe -E _main \
//      hex_const_no_suffix_truncated.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./hex_const_no_suffix_truncated.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, verified
// via -H6 instead, same caveat as this directory's other arm64 repros)

typedef unsigned long long uint64_t64;

extern void exit(int);

const uint64_t64 rndc = 0x800000000000808a;

void
main(void)
{
	if (rndc != 0x800000000000808aULL)
		exit(1);	// the bug: high 32 bits silently zeroed
	exit(0);
}
