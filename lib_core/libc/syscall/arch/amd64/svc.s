// The only raw syscall entry point for linux/amd64: loads up to 6 args
// into the registers the kernel's SYSCALL handler expects (AX=number,
// DI,SI,DX,R10,R8,R9=args -- note R10, not CX: SYSCALL itself clobbers
// CX/R11) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this arch.
//
// note on the FP offsets below: unlike arm64/mips, *no* argument here
// arrives in a register -- confirmed against 6c -S output, the caller
// always writes every argument (including a callee's first) to the
// stack before the call, so this hand-written function can address
// num+0(FP) directly like a compiler-generated one would, with no
// register-vs-FP special case needed for the first argument.
TEXT _syscall6+0(SB), $0
	MOVQ	num+0(FP), AX
	MOVQ	a1+8(FP), DI
	MOVQ	a2+16(FP), SI
	MOVQ	a3+24(FP), DX
	MOVQ	a4+32(FP), R10
	MOVQ	a5+40(FP), R8
	MOVQ	a6+48(FP), R9
	SYSCALL
	RET
