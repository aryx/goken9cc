// setjmp()/longjmp() for linux/arm. See arch/386/setjmp.s's own header
// comment for the general approach (this compiler preserves no
// registers across a call, so only sp+return-address need saving).
//
// Unlike 386/amd64, arm has a dedicated link register (R14) instead of
// a stack-top return address, and its own first argument (j, a
// pointer) DOES arrive unspilled, directly in R0 -- confirmed via
// 5c -S on a probe function, same convention svc_arm.s's own
// _syscall6-adjacent stubs already document ("num arriving in the
// caller's R0, unspilled"). R13/R14 are the real hardware SP/LR here
// (register numbers, not pseudo-tokens like x86's "SP" -- confirmed
// against arch/arm/rt0.s's own direct R13/R14-shaped reasoning, and
// against assemblers/5a/lex.c's own register table). v (longjmp's
// second argument) lands at v+4(FP) -- also confirmed via 5c -S, same
// "first arg's home slot is unused since it stayed in R0, so the next
// argument starts right after it anyway" pattern svc_arm.s's own
// comment already established for a1/a2/a3/a4.
//
// This is a genuine LEAF function (no BL inside either body), so 5a's
// own auto-push-R14-for-any-non-leaf-TEXT-block behavior (see
// arch/arm/rt0.s's own comment on that, found the hard way debugging
// _main's argc/argv offsets) does NOT apply here -- R13/R14 at entry
// are exactly the caller's own values, matching every other $0-frame
// leaf stub already in this tree (svc_arm.s's open/close/create/...).
//
// Ported in spirit from principia's lib_core/libc/arm/setjmp.s (same
// two-field save, same R13/R14 choice -- real ARM link-register
// convention, not this project's own invention), adapted to this
// compiler's own jmp_buf layout and argument offsets.
TEXT setjmp(SB), $0
	MOVW	R13, 0(R0)	// save real sp
	MOVW	R14, 4(R0)	// save return address (the real link register)
	MOVW	$0, R0		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVW	v+4(FP), R1
	CMP	$0, R1
	BNE	ok
	MOVW	$1, R1		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
ok:
	MOVW	0(R0), R2	// saved sp (R0 still holds j here)
	MOVW	4(R0), R14	// saved return address -> restore R14 directly
	MOVW	R1, R0		// return value into R0 (frees R0, j no longer needed)
	MOVW	R2, R13		// restore sp last
	RET			// branches via the now-restored R14, with R0=v
