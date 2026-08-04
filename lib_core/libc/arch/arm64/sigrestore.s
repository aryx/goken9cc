// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/amd64/sigrestore.s's own header comment for the general story
// -- arm64 turns out to be one of the arches where this compiler's own
// first-argument convention already coincides with the real kernel
// ABI (X0), confirmed empirically (an isolated probe, goken's own 7c,
// run under qemu-aarch64): a plain C handler installed DIRECTLY as
// sa.handler already read `sig` correctly, no bridging needed. This
// matches numbers_arm64.h's own earlier finding that this arch's
// kernel supplies its own default restorer with no SA_RESTORER set at
// all -- so, uniquely among the arches here, NEITHER half of the
// usual signal-ABI glue is actually load-bearing on arm64. Still a
// trivial tail-jump rather than removing sigentry from the picture
// entirely, purely so os/linux/notify.c's own installsig() never
// needs a per-arch branch.
TEXT sigentry(SB), $0
	B	signotify(SB)
