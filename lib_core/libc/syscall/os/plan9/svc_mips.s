// Raw Plan9 (GOOS=plan9) syscall stubs for mips. See svc_arm.s's own
// header comment for the general "no shared trampoline needed" story
// -- everything there applies here too, just with this arch's own
// registers: only the *first* named parameter (num) arrives in a
// register (R1, not R0 -- confirmed against linux/mips/svc_mips.s's
// own empirically-verified fact), every *other* argument packs at
// FP+4, FP+8, ... (natural 4-byte slots, first argument's home slot
// still reserved at FP+0), and SYSCALL is the trap instruction.
//
// Unlike linux/mips's own _syscall6 (syscall/os/linux/svc_mips.s),
// which needs an explicit MOVW R2,R1 after SYSCALL because the Linux
// kernel returns its result in R2/$v0 while this compiler's own
// calling convention returns via R1 -- Plan9's kernel returns its
// result directly in R1 (the same register the syscall number went
// in), matching this compiler's convention already (also matching
// machines/vi/syscall.c's own REGRET==1), so no such copy is needed
// here.
//
// Adapted from ~/principia-softwarica/lib_core/libc/9syscall/mkfile's
// per-syscall stub generator -- see svc_arm.s's identical comment for
// the full provenance/verification story, including seek's write-back
// condition (this file uses the same "write back unless R1==-1" logic,
// the mirror image of what principia's own generator emits there too).
#include "sys.h"

// exits(char *msg) -- see svc_arm.s's identical comment.
TEXT exits(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$EXITS, R1
	SYSCALL
	RET

TEXT open(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$OPEN, R1
	SYSCALL
	RET

TEXT close(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$CLOSE, R1
	SYSCALL
	RET

TEXT pread(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$PREAD, R1
	SYSCALL
	RET

TEXT pwrite(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$PWRITE, R1
	SYSCALL
	RET

// see svc_arm.s's identical comment on seek's hidden-return-pointer /
// write-back-unless-error convention -- same story, R1 here instead of
// R0/R2.
TEXT seek(SB), $0
	MOVW	R1, 0(FP)
	MOVW	$SEEK, R1
	SYSCALL
	// this assembler's BEQ only takes one register operand (branch if
	// ==0, see tests/s/features/mips_atomic_llsc.s's own BEQ R6,retry
	// for the same pattern) -- no 2-register-compare form, so compute
	// R1-(-1) first and branch on *that* being zero.
	SUBU	$-1, R1, R3
	BEQ	R3, seekdone
	MOVW	0(FP), R2
	MOVW	R1, 0(R2)
	MOVW	R1, 4(R2)
seekdone:
	RET
