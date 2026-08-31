// setjmp()/longjmp() for linux/sparc. See include/core/exn.h's own
// header comment for the general approach shared by every arch here:
// jmp_buf only ever needs the stack pointer to rewind to (offset 0)
// and the return address to jump back to (offset 4, both uintptr-
// width per struct Jmpbuf).
//
// kc passes the first argument (j, a pointer) in R7 (REGARG=REGRET)
// and the second (v, an int, in longjmp only) on the stack -- at
// v+4(FP), the same convention every other 2-arg stub in this tree
// already uses (see tests/c/vlong/linux_sparc.s's own write() stub).
// REGSP=R1 (k.out.h) is this backend's own logical stack pointer (see
// docs/claude_notes/notes_arch_sparc.txt for the full R14-vs-R1
// story, irrelevant here since this function never touches R14).
// REGLINK=R15 holds the caller's return address, same role as
// power's LR but as an ordinary general register here rather than a
// special-purpose one (SPARC's real "call" instruction format always
// sets %o7=R15 as a side effect, confirmed throughout this arch's own
// bring-up).
//
// Genuine LEAF functions (no JMPL inside either body), so kl's own
// auto-prologue for non-leaf TEXT blocks does not apply -- R15 at
// entry is exactly the caller's own return address, and RETURN's own
// leaf-case expansion ("JMP 8(R15)", confirmed via kl -a on this
// arch's other leaf stubs) picks it up directly once longjmp restores
// it.
TEXT setjmp(SB), $0
	MOVW	R1, 0(R7)	// save real sp (j arrives in R7)
	MOVW	R15, 4(R7)	// save return address into j->pc
	MOVW	$0, R7		// setjmp() returns 0 on the direct call
	RETURN

TEXT longjmp(SB), $0
	MOVW	v+4(FP), R8
	CMP	R8, $0
	BNE	notzero
	MOVW	$1, R8		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOVW	0(R7), R9	// saved sp (R7 still holds j here)
	MOVW	4(R7), R15	// restore R15 = saved return address
	MOVW	R8, R7		// return value into R7 (REGRET), frees R7
	MOVW	R9, R1		// restore sp last
	RETURN			// jumps via the now-restored R15, with R7=v
