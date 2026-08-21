// PowerPC Linux runtime stubs for the float helloprintf test.
//
// Same shape as tests/c/vlong/linux_power.s (see that file's own
// comment for the R3-arg/R2-SB/-0/stack-offset background) plus the
// hardwired-FPU-constant init below.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	// static base (gp = R2) so the $.string(SB) refs in main resolve
	MOVW	$setSB(SB), R2

	// claude: qc reuses F28-F31 (q.out.h: FREGZERO=28/FREGHALF=29/
	// FREGONE=30/FREGTWO=31, see compilers/qc/txt.c's gmove()) as
	// constant registers for common float literals (0.0, 0.5, 1.0,
	// 2.0, 1.5, 2.5, 3.0, -1.0, -2.0, ...) instead of loading them
	// from memory each time. Unlike some archs' float register 0/31,
	// PowerPC has no hardware-guaranteed reset value for any FPR, so
	// all four must be initialized once at boot -- same fix already
	// applied to mips/riscv/alpha in this same file (see
	// linux_mips.s/linux_riscv.s/linux_alpha.s). qa has no
	// float-immediate MOVT (same as za; confirmed empty: no C_FCON
	// row for AFMOVD in linkers/ql/optab.c), so load 0.5 from memory
	// and derive the rest via FADD/FSUB.
	MOVW	$const_half(SB), R3
	FMOVD	(R3), F29           // F29 = FREGHALF = 0.5
	FSUB	F29, F29, F28       // F28 = FREGZERO = 0.5-0.5 = 0.0
	FADD	F29, F29, F30       // F30 = FREGONE  = 0.5+0.5 = 1.0
	FADD	F30, F30, F31       // F31 = FREGTWO  = 1.0+1.0 = 2.0

	// claude: F27 = FREGCVI, a *different* role from F28-31 above --
	// not a literal constant but the magic bias qc's gmove() uses to
	// convert a 32-bit int/uint to double without a native int->float
	// instruction (compilers/qc/txt.c's "fxtofl" label): it builds a
	// double with bit pattern 0x43300000_(f^0x80000000) in memory,
	// loads it, then subtracts FREGCVI (0x4330000080000000 = 2^52 +
	// 2^31) to cancel the bias. Leaving F27 uninitialized (as this
	// file did until real tests/c/float bring-up hit it) makes every
	// int/uint->double conversion subtract ~0 instead, off by the
	// full 2^52+2^31 magnitude -- e.g. a plain "v -= (int)v" on 2.2
	// came out around -4e15 instead of 0.2, since that widening of
	// the truncated int back to double is exactly this fxtofl path.
	MOVW	$const_cvi(SB), R3
	FMOVD	(R3), F27           // F27 = FREGCVI = 2^52 + 2^31

	BL	main(SB)

GLOBL	const_half(SB), $8
DATA	const_half+0(SB)/8, $0.5

GLOBL	const_cvi(SB), $8
DATA	const_cvi+0(SB)/8, $4503601774854144.0

//extern void exit(uint32);
TEXT	exit+0(SB), $0
	// exit code already arrives in R3, which is also where the exit
	// syscall wants it -- nothing to move.
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT	panic+0(SB), $0
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached

//extern void abort(void); // referenced by vlrt.c
TEXT	abort+0(SB), $0
	MOVW	$3, R3          // exit code = 3
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT	write+0(SB), $0
	// fd already arrives in R3, same register the write syscall wants
	// it in -- nothing to move.
	MOVW	buf+4(FP), R4   // buf
	MOVW	count+8(FP), R5 // count
	MOVW	$4, R0          // syscall number = write (4)
	SYSCALL
	RETURN
