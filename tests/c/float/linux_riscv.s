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

	// claude: ic reuses F28/F29/F30/F31 as hardwired 0.0/0.5/1.0/2.0
	// constants for common float literals (see compilers/ic/txt.c
	// gmove(), mirroring compilers/vc/txt.c for mips), instead of
	// loading them from memory. There is no hardwired-constant FPU
	// register on RISC-V (unlike ARM's FPA), so the runtime must
	// initialize them once at boot, the same fix applied to mips in
	// tests/c/float/linux_mips.s. Without this, "small" float constants
	// (0.0, 0.5, 1.0, 2.0, 1.5, 2.5, 3.0, -1.0, -2.0, ...) read as
	// whatever garbage F28-F31 start with (0 here), which is why e.g.
	// 2.0 printed as 0.0 while non-special-cased constants like 2.2
	// (loaded from memory, bypassing this scheme) printed correctly.
	MOVW	$const_half(SB), R8
	MOVD	(R8), F29
	SUBD	F29, F29, F28
	ADDD	F29, F29, F30
	ADDD	F30, F30, F31

	JAL	R1, main(SB)

GLOBL	const_half(SB), $8
DATA	const_half+0(SB)/8, $0.5

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

//extern void abort(void); // referenced by vlrt.c
TEXT	abort+0(SB), $0
	MOVW	$3, R10             // a0 = exit code 3
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
