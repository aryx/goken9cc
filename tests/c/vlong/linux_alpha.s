// Alpha Linux runtime stubs for the vlong helloprintf test.
//
// Same shape as tests/c/mini2/linux_alpha.s (see that file's own
// comment and docs/claude_notes/notes_arch_alpha.txt for the FP-offset/
// SZ_IND background); write() here has the identical 3-arg signature
// so it reuses the same buf+8(FP)/count+16(FP), MOVQ/MOVL split.
//
// Alpha Linux syscall ABI: v0/R0 = syscall number in, result out;
// a0-a5 = R16-R21. CALL_PAL $0x83 ("callsys") traps to the kernel.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	// static base (gp = R29) so the $.string(SB) refs in main resolve
	MOVQ	$setSB(SB), R29
	JSR	,main+0(SB)

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
