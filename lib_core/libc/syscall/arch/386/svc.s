// The only raw syscall entry point for linux/386: loads up to 6 args
// into the registers the kernel's INT $0x80 handler expects (AX=number,
// BX,CX,DX,SI,DI,BP=args) and traps. Every OS/arch's syscall wrappers
// (see syscall/os/$OS/) are generated thin C functions calling this,
// so this is the one place that ever needs hand-written assembly for
// this arch.
//
// note on the FP offsets below: like amd64, no argument here arrives
// in a register -- confirmed against 8c -S output, the caller always
// writes every argument (including a callee's first) to the stack
// before the call, so this hand-written function can address
// num+0(FP) directly like a compiler-generated one would.
//
// no explicit return-value copy needed: AX is both this arch's C ABI
// return register and where INT $0x80 leaves the syscall's result,
// unlike the RISC arches here whose C return register differs from
// their syscall-number register.
TEXT _syscall6+0(SB), $0
	MOVL	num+0(FP), AX
	MOVL	a1+4(FP), BX
	MOVL	a2+8(FP), CX
	MOVL	a3+12(FP), DX
	MOVL	a4+16(FP), SI
	MOVL	a5+20(FP), DI
	MOVL	a6+24(FP), BP
	INT	$0x80
	RET
