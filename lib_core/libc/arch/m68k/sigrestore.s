// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/mips/sigrestore.s's own header comment for the general story
// this file follows.
//
// Unlike sparc/mips's own sigrestore.s (both need to move `sig` from
// a hardware first-argument REGISTER into the register/stack slot
// their own compiler's calling convention expects), m68k needs no
// such bridge: 2c's own calling convention already passes every
// argument on the stack (no REGARG at all -- see docs/claude_notes/
// notes_arch_m68k.txt), and the real m68k C ABI the Linux kernel's
// own signal-frame setup targets is ALSO stack-based, so `sig` should
// already land exactly where signotify(int sig) expects it (0(FP)) --
// UNVERIFIED (no signal test has run on this arch yet), but this is
// the natural, no-bridge-needed consequence of both sides already
// agreeing on a stack-passed first argument, unlike sparc/mips where
// the mismatch is real and confirmed.
//
// No manual return-address save/restore needed around the BSR: real
// m68k BSR/RTS already use the actual hardware stack, so the trailing
// RTS correctly returns to whatever the kernel pushed there before
// jumping here -- same "no manual restorer needed" assumption
// arch/mips/sigrestore.s's own comment makes.
TEXT sigentry(SB), $0
	BSR	signotify(SB)
	RTS
