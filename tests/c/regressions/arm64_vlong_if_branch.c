// arm64_vlong_if_branch: found while porting benchs/compcert/vmach.c to
// build under goken's own toolchain (after widening its `long
// stack[]`/`sp` to `long long`, needed since this compiler's `long`
// stays 4 bytes even on 64-bit arches -- see notes_libc_selfhost.txt).
// The widened file hit a *linker* diagnostic that looked like a 7l bug
// ("illegal combination MOV REG NONE SBRA"), but is actually a 7c
// (compiler) bug: `if (some_vlong_expr)` -- a raw truthiness test on a
// `long long`/`vlong` value, written with no explicit comparison
// operator -- silently emits NO compare-and-branch instruction at all
// on arm64. The "then" branch's code ends up unconditionally spliced
// into the instruction stream right after whatever came before the
// "if", and the linker diagnostic was just a side effect: 7l's span.c
// dutifully reports the corrupted Prog it was actually given, so
// fixing 7c also makes that diagnostic (and the whole detour of
// suspecting 7l/span.c or an "sp" vs SP-register naming collision --
// both dead ends, see git history) disappear on its own.
//
// Root cause: src/cmd/cc/pgen.c's bcomplex() (used by every "if"/"for"/
// "while" condition, in gen()'s OIF case) has a dedicated fast path for
// vlong conditions on archs with native 64-bit support (`typev[...] &&
// machcap(Z)`, true for every 64-bit-native backend including arm64):
// it synthesizes a `left != 0` comparison node and calls `cgen(b, Z)`.
// compilers/7c/cgen.c's own OEQ/ONE/... case, when handed nn==Z (as it
// always is here), just does `nullwarn(l, r); break;` -- correct for
// its OTHER caller (a genuine "x == y;" statement whose result is
// unused), but here it means NO code gets emitted at all. bcomplex()'s
// caller in turn grabs the compiler's "last emitted instruction"
// pointer right after, assuming (correctly, for the *fallback* path
// two lines below, `bool64(n); boolgen(n, 1, Z);`) that a pending
// conditional branch is there to backpatch once its target address is
// known -- but with nothing emitted, that pointer is left dangling on
// whatever unrelated instruction preceded the "if", and the later
// backpatch corrupts *that* instruction instead.
//
// Confirmed 7c-specific (not shared kencc-lineage code, despite
// bcomplex() itself living in the shared src/cmd/cc/pgen.c): 6c/amd64
// hits the exact same `typev[...] && machcap(Z)` condition but takes a
// visibly different route -- its own `compilers/cck/pgen.c` fork of
// bcomplex() already calls `boolgen(b, 1, Z)` directly instead of
// `cgen(b, Z)`, sidestepping cgen()'s nn==Z bail-out entirely (compared
// -g codegen traces on an identical repro to confirm). Fixed by making
// src/cmd/cc/pgen.c's bcomplex() do the same: boolgen() always emits
// the compare+branch unconditionally, only gating its own
// value-materialization epilogue on nn!=Z, so it leaves the compiler's
// "last emitted instruction" state in the shape the OIF caller needs.
//
// This checks actual runtime *behavior*, not just "did it link": before
// the fix, `sp++` executed unconditionally (no branch at all), so
// pick(&stack[0]) wrongly returned stack[2] instead of stack[0].
//
//   7c -c arm64_vlong_if_branch.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_vlong_if_branch.exe -E _main \
//      arm64_vlong_if_branch.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_vlong_if_branch.exe; echo $?
// (want 0; this session's own darwin/arm64 host has no qemu, verified
// via -H6 instead, same caveat as this directory's other arm64 repros)

long long stack[4] = {0, 111, 222, 0};

extern void exit(int);

long long
pick(long long *sp)
{
	long long arg;

	arg = *sp;
	if (arg)
		sp++;
	return *sp;
}

void
main(void)
{
	if (pick(&stack[0]) != 0)
		exit(1);	// if(0) wrongly took the branch (sp++ ran unconditionally)
	if (pick(&stack[1]) != 222)
		exit(2);	// if(111) should also branch -- sanity check, not the bug itself
	exit(0);
}
