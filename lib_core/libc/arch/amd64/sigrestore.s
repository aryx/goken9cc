// Linux signal-ABI glue for amd64, Tier 6 notification
// (docs/claude_notes/plan_syscalls.txt). Two trampolines, both because
// this compiler's own calling convention does not match the raw
// kernel ABI it has to interoperate with here (see each one's own
// comment for the specific mismatch).

// sigentry() -- installed as Ksigaction.handler (NOT os/linux/
// notify.c's own signotify() directly -- see that file's own comment
// on sigentry for the real bug this fixes, found empirically: a
// handler installed directly always observed sig=1 regardless of
// which signal actually fired). The kernel invokes a signal handler
// using the real amd64 SysV ABI: first int arg in EDI/RDI, a fixed
// convention every signal-capable program on this kernel must match.
// But THIS compiler's own C calling convention is stack-based even on
// amd64 (arch/amd64/rt0.s's own comment: "amd64 has no register-passed
// argument at all... every argument, including a callee's first, is
// written to the stack by the *caller*") -- so signotify(int sig),
// installed directly, would read `sig` from a stack slot the kernel
// never wrote anything into. This bridges the two, the same technique
// rt0.s already uses to bridge the kernel's raw process-entry
// convention into a proper call to main(): read sig from the real
// ABI's register, write it to the stack slot THIS compiler's CALL
// convention expects (mirroring rt0.s's own SUBQ/MOVQ/CALL shape),
// then call signotify normally. The kernel's own sigreturn-frame
// return address (pushed before jumping here, exactly like a normal
// CALL would) is left untouched by the SUBQ/ADDQ pair, so sigentry's
// own RET at the end pops it and transfers control to sigrestorer
// below -- the real mechanism by which SA_RESTORER-based signal return
// actually works on this ABI, not something sigentry has to do
// explicitly itself.
TEXT sigentry(SB), $0
	SUBQ	$8, SP
	MOVL	DI, 0(SP)
	CALL	signotify(SB)
	ADDQ	$8, SP
	RET

// sigrestorer() -- the Linux rt_sigreturn(2) trampoline
// Ksigaction.restorer (numbers_amd64.h) must point at. The kernel's
// rt_sigaction(2) does not synthesize a default return-from-handler
// sequence itself on this arch (unlike arm64, empirically confirmed
// to supply its own -- see numbers_arm64.h's comment) -- userspace has
// to hand it one via SA_RESTORER, the same "restorer" every libc
// (glibc's __restore_rt, musl's __restore) supplies internally. Never
// called directly by any C code and never returns to its own caller
// (rt_sigreturn hands control back to whatever was interrupted,
// restoring the saved signal mask/registers from the kernel's own
// sigframe) -- reached only via sigentry's own RET above transferring
// control here (the real ABI mechanism, not an explicit jump), so no
// C-callable argument/return shape applies here at all.
TEXT sigrestorer(SB), $0
	MOVQ	$15, AX		// __NR_rt_sigreturn (numbers_amd64.h)
	SYSCALL
	// unreachable: rt_sigreturn never returns here
