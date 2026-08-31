// Linux SPARC32 runtime stubs for the mini2 helloprintf test.
//
// Same register/stack conventions already established and verified by
// tests/c/mini's start_sparc.s/xwrite_sparc.s/xexit_sparc.s (see those
// files' own comments, and docs/claude_notes/notes_arch_sparc.txt, for
// the full story on why R1/REGSP needs the R14->R1 copy at _main, why
// _main must stay leaf (register-indirect JMP, not JMPL, to main), and
// why R1 has to be saved/restored around any syscall that returns
// (R1/REGSP collides with the real hardware %g1 syscall-number
// register). write() here has a THIRD arg (fd, buf, count all real
// parameters, unlike xwrite_sparc.s's fd=1 hardcoded) -- 'kc -S' on a
// hand-written 3-arg function confirms fd arrives in R7 (REGARG), buf
// at buf+4(FP), count at count+8(FP), same FP-relative stack
// convention as xwrite_sparc.s's own count+4(FP) for its 2nd arg.

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
