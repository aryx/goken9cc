// RISC-V64 (rv64) Linux runtime stubs for the mini2 helloprintf test.
// Same as linux_riscv.s (rv32) except for the FP-relative stack-argument
// offsets: jc passes the first argument in R8 (REGARG) and the rest on
// the stack, one ptrsize-wide slot per argument (see linkers/il/span.c).
// ptrsize is 8 here (vs 4 for rv32), so the 2nd/3rd args land at 8(FP)/
// 16(FP) instead of rv32's 4(FP)/8(FP).

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
	MOV	buf+8(FP), R11      // a1 = buf
	MOVW	count+16(FP), R12   // a2 = count
	MOVW	$64, R17            // a7 = syscall number: write
	ECALL
	RET
