// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification. See arch/amd64/sigrestore.s's own header
// comment for the general story -- power, like arm, is one of the
// arches where this compiler's own first-argument convention already
// coincides with the real kernel ABI (R3=REGARG, the standard C ABI
// register the kernel also delivers `sig` in), so a plain C handler
// installed directly as sa.handler should read `sig` correctly with
// no bridging needed, matching arm's own (tested) finding -- not yet
// independently verified for power with a real kill(2)+notify()
// probe. This is a trivial tail-jump rather than removing sigentry
// from the picture entirely, purely so os/linux/notify.c's own
// installsig() never needs a per-arch branch.
TEXT sigentry(SB), $0
	BR	signotify(SB)
