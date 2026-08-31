// Linux SPARC32 runtime stubs for the vlong helloprintf test.
//
// Same shape as tests/c/mini2/linux_sparc.s (see that file's own
// comment, and docs/claude_notes/notes_arch_sparc.txt, for the full
// R14->R1/leaf-_main/REGSP-save-around-syscall story) plus one extra
// stub, abort(), referenced by vlrt.c on an internal consistency
// error.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	MOVW R14, R1
	MOVW $setSB(SB), R2
	MOVW $main(SB), R8
	JMP (R8)

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
