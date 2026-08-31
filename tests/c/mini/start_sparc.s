// start_sparc.s -- Linux SPARC32 process entry point for helloc.exe.
//
// R14 (REGTMP, "used by the loader") IS the real hardware %sp/%o6 --
// this backend's own REGSP=1=%g1 is a completely separate register
// with no relation to the OS-provided stack (see docs/claude_notes/
// notes_arch_sparc.txt: a flat, non-windowed calling convention that
// repurposes %sp as an ordinary scratch temp for sethi/add constant
// loads, e.g. setSB's own load below). So unlike start_mips.s/start_
// alpha.s/start_riscv.s (whose REGSP already IS the OS-initialized
// register, needing no setup here), this port must copy the kernel's
// initial %sp into R1 (REGSP) itself, and do it as the VERY FIRST
// instruction -- setSB's sethi would otherwise clobber R14 first and
// lose it. Confirmed against a real bug: without this, "SUB $8,R1"
// in main's prologue underflows R1 from 0 to -8 (0xfffffff8) and the
// very first store through it segfaults before any syscall runs.
//
// setSB then loads R2 (REGSB, this port's "static pointer") with the
// runtime data-segment base, needed for the string references kc
// generates in main -- same role as setSB in start_alpha.s / start_
// mips.s's setR30.
//
// kc lowers a C function call to AJMPL (real SPARC "jmpl"/"call", not
// ABL -- unlike qc/PowerPC, ABL here is a genuine conditional branch
// opcode (OLT's "branch if less"), see compilers/kc/txt.c's OFUNC/OLT
// cases, so it can't double as the call pseudo-op the way it does on
// power). But an AJMPL here would defeat the R14->R1 setup above: kl's
// noop.c unconditionally clears LEAF on any AJMPL in a TEXT (no check
// of the target register) and inserts its own "SUB $8,R1 / MOVW
// R15,0(R1)" prologue at the very top of the TEXT, ahead of anything
// written here in source order -- so the auto-prologue would still
// run first and stomp R1 before this file's own R14->R1 copy got a
// chance to. _start never needs to return to itself anyway (control
// passes to main permanently), so a genuine tail-jump avoids the
// prologue instead: load main's address into a scratch register, then
// jump through it with the register-indirect AJMP form (unlike AJMPL,
// AJMP doesn't clear LEAF -- see noop.c), same "JMP offset(Rreg)" form
// RETURN itself expands to.

TEXT _start(SB), $0
	MOVW R14, R1
	MOVW $setSB(SB), R2
	MOVW $main(SB), R8
	JMP (R8)
