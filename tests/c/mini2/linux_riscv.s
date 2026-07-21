// RISC-V (rv32) Linux runtime stubs for the mini2 helloprintf test.
//
// ic passes the first argument in R8 (REGARG) and the rest on the stack;
// the Linux RISC-V syscall ABI wants a0=R10, a1=R11, a2=R12 and the
// syscall number in a7=R17 (write=64, exit=93), trapping with ECALL.
// FP params resolve to autosize+offset+ptrsize (see linkers/il/span.c);
// these are all leaves (autosize=0) and ptrsize=4 (rv32), so the 2nd/3rd
// args the caller stores at 8(SP)/12(SP) are read at 4(FP)/8(FP).

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	// static base (gp = R3) so the $.string(SB) refs in main resolve
	MOVW	$setSB(SB), R3
	JAL	R1, main(SB)

//extern void exit(uint32);
TEXT	exit+0(SB), $0
	MOVW	R8, R10             // a0 = exit code (1st arg arrives in R8)
	MOVW	$93, R17            // a7 = syscall number: exit
	ECALL
	RET // never reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT	panic+0(SB), $0
	MOVW	R8, R10             // a0 = exit code
	MOVW	$93, R17            // a7 = syscall number: exit
	ECALL
	RET // never reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT	write+0(SB), $0
	MOVW	R8, R10             // a0 = fd (1st arg arrives in R8)
	MOVW	buf+4(FP), R11      // a1 = buf
	MOVW	count+8(FP), R12    // a2 = count
	MOVW	$64, R17            // a7 = syscall number: write
	ECALL
	RET
