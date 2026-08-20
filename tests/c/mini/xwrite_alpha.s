TEXT xwrite+0(SB), $0
	// xwrite(char *buf, int count) -> write(1, buf, count).
	// zc passes the first arg (buf) in R0 (REGARG=REGRET=0) and stores
	// count on the stack at count+8(FP) -- confirmed with 'zc -S' on a
	// two-arg function, not derived from the FP-offset formula the
	// other archs' xwrite_*.s comment (autosize+offset+ptrsize): zc
	// has no notion of ptrsize (unlike the dual-width ic/il). The
	// register-passed first arg (buf, a pointer) still reserves a
	// full 8-byte home slot in the frame, pushing count to +8 rather
	// than +4 -- +4 was this file's own original value, read off zc
	// -S *before* compilers/zc/gc.h's SZ_IND=4 bug (pointers declared
	// 4 bytes wide on a machine where they're 8 -- see
	// docs/claude_notes/notes_arch_alpha.txt) was fixed; it changed
	// zc's own frame layout, so this stub had to be re-checked against
	// zc -S again, not just left as "already confirmed working".
	//
	// Alpha Linux syscall ABI: v0/R0 = syscall number in, result out;
	// a0-a5 = R16-R21. R0 must be moved out to a1 before it's
	// clobbered with the syscall number (same reason arm64's xwrite
	// moves its own R0 to R1 first).
	MOVL	count+8(FP), R18    // a2 = count
	MOVQ	R0, R17             // a1 = buf (arrives in R0)
	MOVQ	$1, R16             // a0 = fd = 1
	MOVQ	$4, R0              // v0 = syscall number: write (4)
	CALL_PAL $0x83              // callsys
	RET
