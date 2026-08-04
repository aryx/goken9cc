// setjmp()/longjmp() for linux/arm64. See arch/arm/setjmp.s's own
// header comment for the general approach and the R0-unspilled-first-
// argument story (confirmed the same way here, via 7c -S on a probe
// function). RSP is this arch's own real-hardware-stack-pointer name
// (assemblers/7a/lex.c's own register table; SP is a genuine
// AArch64-ISA-special register, unlike 386/amd64's, so this assembler
// gives it a distinct symbolic name rather than overloading "SP" the
// way it does for the FP-relative-locals pseudo-token) -- already used
// this way, unmodified, throughout arch/arm64/rt0.s. LR is R30's own
// alias (same lex.c table) -- the real AArch64 link register. v
// (longjmp's second argument) lands at v+8(FP), the uniform 8-byte-
// slot convention this arch already uses for every argument after the
// first (same story arm's own 4-byte packing has, just wider).
//
// Genuine LEAF function (no BL inside either body), so 7a's own
// auto-16-byte-aligned-LR-saving prologue for non-leaf TEXT blocks
// (arch/arm64/rt0.s's own comment) does not apply -- RSP/LR at entry
// are exactly the caller's own values, matching every other $0-frame
// leaf stub already in this tree (svc_arm64.s's _syscall6 and its
// siblings). Uses RETURN (this arch's own return pseudo-op, not RET --
// already established by svc_arm64.s and every other arm64 .s file in
// this tree) and CBZ (branch-if-zero, already proven working in
// tests/s/features/arm64_atomic_ldxr_stxr.s) for the ansi
// longjmp(j, 0) special case, avoiding any assumption about CMP's
// exact operand order on this arch.
//
// claude: setjmp() saves RSP via an intermediate register (MOV RSP,R2
// then MOV R2,0(R0)) rather than storing it directly -- found via a
// real segfault (a working test on every OTHER arch here, arm64 alone
// corrupting its own restored sp to 0) and root-caused with `7l -a`'s
// disassembly listing, not guessed: `MOV RSP, 0(R0)` assembles to a
// plain STR with Rt=31, but AArch64's ISA hardwires register 31 to
// mean XZR (the zero register) in a store's *value* field -- it only
// means SP in specific ADD/SUB-immediate forms (the "mov to/from sp"
// idiom real ARM64 code always uses instead of a direct store). So
// that line was silently storing zero, not the real stack pointer.
// The restore direction (`MOV R2, RSP`, disassembling to an ADD
// R2,SP,#0) has no such ambiguity -- R31 there is unambiguously SP in
// ADD-immediate's own encoding -- confirmed correct in the same
// disassembly and left unchanged.
TEXT setjmp(SB), $0
	MOV	RSP, R2
	MOV	R2, 0(R0)	// save real sp (via R2 -- see comment above)
	MOV	LR, 8(R0)	// save return address (the real link register)
	MOV	$0, R0		// setjmp() returns 0 on the direct call
	RETURN

TEXT longjmp(SB), $0
	MOV	v+8(FP), R1
	CBZ	R1, zero
	B	ok
zero:
	MOV	$1, R1		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
ok:
	MOV	0(R0), R2	// saved sp (R0 still holds j here)
	MOV	8(R0), LR	// saved return address -> restore LR directly
	MOV	R1, R0		// return value into R0 (frees R0, j no longer needed)
	MOV	R2, RSP		// restore sp last
	RETURN			// branches via the now-restored LR, with R0=v
