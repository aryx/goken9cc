// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/amd64/sigrestore.s's own header comment for the general story
// -- arm turns out to be one of the arches where this compiler's own
// first-argument convention already coincides with the real kernel
// ABI (R0), confirmed empirically (an isolated probe, goken's own 5c,
// run under qemu-arm): a plain C handler installed DIRECTLY as
// sa.handler already read `sig` correctly, no bridging needed, no
// SA_RESTORER needed either (tested and confirmed: the kernel's own
// default return mechanism works). This is a trivial tail-jump rather
// than removing sigentry from the picture entirely, purely so
// os/linux/notify.c's own installsig() never needs a per-arch branch.
TEXT sigentry(SB), $0
	B	signotify(SB)
