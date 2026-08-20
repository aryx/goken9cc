// Alpha Linux runtime stubs for the mini2 helloprintf test.
//
// zc passes the first argument in R0 (REGARG=REGRET=0) and the rest on
// the stack; confirmed with 'zc -S' on an equivalent 3-arg function,
// not derived from the autosize+offset+ptrsize formula the riscv/
// arm64 stubs' comments use (zc has no ptrsize concept, unlike the
// dual-width ic/il). buf (a pointer) lands at 8(FP), loaded with MOVQ;
// count at 16(FP), loaded with MOVL. These offsets/widths reflect
// compilers/zc/gc.h's SZ_IND now being correctly 8 (see
// docs/claude_notes/notes_arch_alpha.txt) -- an earlier version of
// this file used 4(FP)/8(FP) with MOVL for buf too, read off zc -S
// *before* that fix, when zc's own frame layout (wrongly) treated
// pointers as 4 bytes; re-check with zc -S again if gc.h's SZ_IND or
// the argument shape here ever changes, rather than assuming old
// offsets still hold.
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

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT	write+0(SB), $0
	MOVQ	R0, R16             // a0 = fd (1st arg arrives in R0)
	MOVQ	buf+8(FP), R17      // a1 = buf
	MOVL	count+16(FP), R18   // a2 = count
	MOVQ	$4, R0              // v0 = syscall number: write (4)
	CALL_PAL $0x83
	RET
