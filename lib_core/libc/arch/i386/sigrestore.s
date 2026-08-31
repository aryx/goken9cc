// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/amd64/sigrestore.s's own header comment for the general story
// (this compiler's C calling convention vs. the raw kernel ABI it has
// to interoperate with here). On i386, the kernel passes sig on the
// STACK (cdecl has no register-passed args at all), at the same
// offset THIS compiler's own convention would put it -- so, unusually,
// argument reading alone would have worked with signotify() installed
// directly (confirmed by an isolated probe). But a plain JMP straight
// to signotify(SB) as sa.handler crashes immediately (confirmed by an
// even more isolated probe: a JMP to an external symbol, installed as
// the actual kernel-invoked handler, segfaults on entry every time --
// this is NOT the same as a same-file `JMP label` like arch/386/rt0.s's
// own `JMP loop`, which is just a local PC-relative branch; jumping to
// yet another compilation unit's external symbol this way does not
// work as a raw kernel entry point). So this still needs a real
// bridge, just a differently-shaped one than amd64's: stage sig
// through a proper CALL (so signotify sees it at the plain stack
// offset its own prologue expects), then RET -- twice, once to unwind
// signotify's own CALL-pushed return address, once more to reach
// whatever the kernel originally set up as ITS return point.
//
// claude: NO restorer here, and this arch must NOT get one -- tested
// independently from the argument-bridging fix above (they are two
// separate bugs, found and fixed one at a time, not one combined fix):
// with SA_RESTORER/a real rt_sigreturn trampoline (this file's own
// prior content), this exact bridging logic reliably segfaults on
// resume; without it (confirmed via qemu-i386 strace), the kernel
// falls back to its own legacy sigreturn(2) path, which works
// correctly with this trampoline. Root cause of the restorer-specific
// crash not fully chased down (suspected i386 rt_sigframe layout
// mismatch), but the working/broken split itself is real and
// reproduced multiple times, not a fluke -- see os/linux/notify.c's
// own comment on sigentry for the fuller cross-arch picture.
TEXT sigentry(SB), $0
	MOVL	4(SP), AX
	SUBL	$4, SP
	MOVL	AX, 0(SP)
	CALL	signotify(SB)
	ADDL	$4, SP
	RET
