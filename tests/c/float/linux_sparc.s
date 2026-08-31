// Linux SPARC32 runtime stubs for the float helloprintf test.
//
// Same shape as tests/c/vlong/linux_sparc.s (see that file's own
// comment, and docs/claude_notes/notes_arch_sparc.txt, for the R14->
// R1/leaf-_main/REGSP-save-around-syscall background) plus the
// hardwired-FPU-constant init below.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	MOVW R14, R1
	MOVW $setSB(SB), R2

	// claude: kc reuses F24/F26/F28/F30 (k.out.h: FREGZERO=24/
	// FREGHALF=26/FREGONE=28/FREGTWO=30, both float and double, see
	// compilers/kc/txt.c's gmove()) as constant registers for common
	// float literals instead of loading them from memory each time,
	// same convention as qc/PowerPC's own FREGZERO/HALF/ONE/TWO (see
	// this dir's own linux_power.s comment) -- no hardware-guaranteed
	// reset value for any FPR, so all four must be initialized once
	// at boot. Unlike qc, kc has no
	// FREGCVI-equivalent bias-constant register: SPARC has real
	// hardware FITOD/FITOS int->float conversion instructions
	// (AFMOVWD/AFMOVWF, see kc/txt.c's fxtofl: label), so no software
	// bias trick is needed there. ka has no float-immediate FMOVD (no
	// C_FCON row for AFMOVD in linkers/kl/optab.c, same gap as qa/za),
	// so load 0.5 from memory and derive the rest via FADDD/FSUBD.
	MOVW	$const_half(SB), R3
	FMOVD	(R3), F26           // F26 = FREGHALF = 0.5
	FSUBD	F26, F26, F24       // F24 = FREGZERO = 0.5-0.5 = 0.0
	FADDD	F26, F26, F28       // F28 = FREGONE  = 0.5+0.5 = 1.0
	FADDD	F28, F28, F30       // F30 = FREGTWO  = 1.0+1.0 = 2.0

	MOVW $main(SB), R8
	JMP (R8)

GLOBL	const_half(SB), $8
DATA	const_half+0(SB)/8, $0.5

//extern void exit(uint32);
TEXT exit+0(SB), $0
	MOVW	R7, R8      // status (arrives in R7/REGARG) -> o0
	MOVW	$1, R1      // g1 = syscall number = exit (1)
	MOVW	$0x10, R9   // scratch reg holding the trap number
	TA	R9
	RETURN              // not reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT panic+0(SB), $0
	MOVW	R7, R8      // status (arrives in R7/REGARG) -> o0
	MOVW	$1, R1      // g1 = syscall number = exit (1)
	MOVW	$0x10, R9   // scratch reg holding the trap number
	TA	R9
	RETURN              // not reached

//extern void abort(void); // referenced by vlrt.c
TEXT abort+0(SB), $0
	MOVW	$3, R8      // exit code = 3
	MOVW	$1, R1      // g1 = syscall number = exit (1)
	MOVW	$0x10, R9   // scratch reg holding the trap number
	TA	R9
	RETURN              // not reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT write+0(SB), $0
	MOVW	R7, R8           // fd (arrives in R7/REGARG) -> o0
	MOVW	buf+4(FP), R9    // buf -> o1
	MOVW	count+8(FP), R10 // count -> o2
	MOVW	R1, R12          // save REGSP before clobbering it below
	MOVW	$4, R1           // g1 = syscall number = write (4)
	MOVW	$0x10, R11       // scratch reg holding the trap number
	TA	R11
	MOVW	R12, R1          // restore REGSP
	MOVW	R8, R7           // syscall return (o0) -> REGRET
	RETURN
