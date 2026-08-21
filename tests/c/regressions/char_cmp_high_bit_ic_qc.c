// char_cmp_high_bit_ic_qc: found while writing demos/hello_unix.c's
// sbrk() write+readback check (a byte-pattern loop very close in shape
// to tests/c/hello_libc/mem.c's own, except mem.c stays under 128 and
// this one didn't). On riscv (ic) and power (qc) only, comparing a
// `char` holding a value >= 128 against a runtime-computed `(char)`
// cast expression with `==`/`!=` gives the WRONG answer, even though
// both sides hold the identical bit pattern and print identically via
// %d. No loop is needed to trigger it -- see the minimal repro below,
// isolated down from the original write-then-compare loop.
//
// Confirmed real: with i=127 (not a compile-time constant -- read from
// a variable, matching how a real loop index behaves), `buf[0] =
// (char)(i+1)` stores 128, which as a signed char is -128; `v =
// buf[0]` reads the same -128 back; yet `v == (char)(i+1)` -- both
// operands the literal value -128 -- evaluates to 0 (false) on ic and
// qc. exit(1) below, not exit(0), before this is fixed.
//
//     ic:  eq=0 (wrong)      qc:  eq=0 (wrong)
//     7c:  eq=1 (correct)    8c:  eq=1 (correct)    6c:  eq=1 (correct)
//     5c, vc: not checked (this session's own arm/mips builds under
//       qemu-arm/qemu-mips either crashed or hung when probed the same
//       way -- an unrelated setup issue in this environment, not
//       evidence either way for 5c/vc's own correctness here)
//
// Root cause not yet isolated inside ic/cgen.c or qc/cgen.c's boolgen()/
// relational-operator codegen (see docs/claude_notes/
// notes_shared_frontend_bugs.txt's own boolgen()-related entry for a
// DIFFERENT bug in the same function, unsound NaN handling under
// floating-point comparisons on these same two backends -- worth
// checking whether the two share a cause, given both live in the
// vc/ic-specific comrel()/boolgen() machinery, but not confirmed).
// Likely candidate: a byte value loaded from memory being compared at
// a different width or sign-extension than a same-valued byte produced
// by a runtime cast, without the two being normalized to the same
// representation first -- but this is a hypothesis, not a source-level
// finding.
//
// Not wired into this directory's `test:V:` (see mkfile's own comment):
// still an open bug, not a fixed one to guard. Verify by hand once a
// fix lands -- these two archs need a real libc.a rebuilt first (their
// runtime relies on exit()/print(), unlike most of this directory's
// other minimal repros, which get by with a hand-written _main/exit
// stub instead):
//   cd ../../../lib_core/libc && rm -f libc.a && mk -a objtype=riscv cputype=riscv install
//   cd ../../tests/c/regressions
//   ic -I../../../include -I../../../include/ALL -I../../../include/arch/riscv -Driscv -c char_cmp_high_bit_ic_qc.c
//   il -L../../../ROOT/arch/riscv/lib -H7 -o char_cmp_high_bit_ic_qc_riscv.exe -E _main char_cmp_high_bit_ic_qc.i -lc
//   ../../../scripts/qemu-runner riscv ./char_cmp_high_bit_ic_qc_riscv.exe; echo $?
// (want 0; got 1 before any fix -- same recipe with
// 'objtype=power cputype=power', qc -0/ql -0 -s/-Dpower/-I.../arch/power
// and qemu-runner power reproduces it on power too)

#include <u.h>
#include <libc.h>

void
main(void)
{
	int i;
	char buf[4], v;

	i = 127;
	buf[0] = (char)(i + 1);
	v = buf[0];
	if (v != (char)(i + 1)) {
		print("char_cmp_high_bit: v=%d cast=%d -- should be equal\n",
			v, (char)(i + 1));
		exit(1);
	}
	exit(0);
}
