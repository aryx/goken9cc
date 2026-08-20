// Alpha Linux runtime stubs for the float helloprintf test.
//
// Same shape as tests/c/vlong/linux_alpha.s (see that file's own
// comment and docs/claude_notes/notes_arch_alpha.txt for the FP-offset/
// SZ_IND background) plus the hardwired-FPU-constant init below.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	// static base (gp = R29) so the $.string(SB) refs in main resolve
	MOVQ	$setSB(SB), R29

	// claude: zc reuses F28/F29/F30 (plus F31, architecturally
	// hardwired to +0.0 on real Alpha silicon -- see z.out.h's
	// FREGZERO=31/FREGHALF=28/FREGONE=29/FREGTWO=30, and compilers/zc/
	// txt.c's gmove()) as constant registers for common float literals
	// (0.0, 0.5, 1.0, 2.0, 1.5, 2.5, 3.0, -1.0, -2.0, ...), instead of
	// loading them from memory each time. Unlike F31, F28-F30 are
	// ordinary registers with no hardware-guaranteed reset value, so
	// they must be initialized once at boot -- the same fix already
	// applied to mips/riscv in this same file (see linux_mips.s/
	// linux_riscv.s). za has no float-immediate MOVT (unlike mips'
	// assembler sugar for "MOVD $0.5,F26"; confirmed empty in
	// linkers/lk/optab.c: no C_FCON row for AMOVT), so load 0.5 from
	// memory (as riscv also has to) and derive the rest via ADDT.
	MOVQ	$const_half(SB), R1
	MOVT	(R1), F28
	ADDT	F28, F28, F29
	ADDT	F29, F29, F30

	JSR	,main+0(SB)

GLOBL	const_half(SB), $8
DATA	const_half+0(SB)/8, $0.5

//extern void exit(uint32);
TEXT	exit+0(SB), $0
	MOVQ	R0, R16             // a0 = exit code (1st arg arrives in R0)
	MOVQ	$1, R0              // v0 = syscall number: exit (1)
	CALL_PAL $0x83
	RET // never reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT	panic+0(SB), $0
	MOVQ	R0, R16             // a0 = exit code
	MOVQ	$1, R0              // v0 = syscall number: exit (1)
	CALL_PAL $0x83
	RET // never reached

//extern void abort(void); // referenced by vlrt.c
TEXT	abort+0(SB), $0
	MOVQ	$3, R16             // a0 = exit code 3
	MOVQ	$1, R0              // v0 = syscall number: exit (1)
	CALL_PAL $0x83
	RET // never reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT	write+0(SB), $0
	MOVQ	R0, R16             // a0 = fd (1st arg arrives in R0)
	MOVQ	buf+8(FP), R17      // a1 = buf
	MOVL	count+16(FP), R18   // a2 = count
	MOVQ	$4, R0              // v0 = syscall number: write (4)
	CALL_PAL $0x83
	RET
