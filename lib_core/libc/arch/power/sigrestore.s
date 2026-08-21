// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification. See arch/amd64/sigrestore.s's own header
// comment for the general story -- power, like arm, is one of the
// arches where this compiler's own first-argument convention already
// coincides with the real kernel ABI (R3=REGARG, the standard C ABI
// register the kernel also delivers `sig` in), so a plain C handler
// installed directly as sa.handler should read `sig` correctly with
// no argument bridging needed, matching arm's own (tested) finding --
// not yet independently verified for power with a real
// kill(2)+notify() probe.
//
// Unlike arm's own sigentry (a bare tail-branch, "B signotify(SB)"),
// this one is a real BL+manual-LR-restore, not a tail-jump: ql's own
// ABR only has an optab row for a same-object/local branch target
// (C_LBRA -- confirmed via linkers/ql/optab.c, and ABL shares that
// same row, oprange[ABL]=oprange[ABR] in span.c), and "BR
// signotify(SB)" -- signotify lives in a DIFFERENT compilation unit,
// pulled from libc.a's own archive scan -- failed to assemble
// ("illegal combination BR NONE NONE NONE LEXT") the one time this
// was tried as a plain tail-branch, unlike "BL exit(SB)"/"BL main(SB)"
// (arch/power/rt0.s), which cross the same file boundary successfully.
// Root cause not chased further (looks like an archive-resolution-
// order quirk specific to BR, not a fundamental "external targets
// need C_LEXT" gap, since BL already proves cross-file linking works)
// -- worked around instead of fixed, since notify() has no other
// caller needing "BR to an external libc.a symbol" fixed properly.
//
// LR at entry holds the kernel's own default signal-return address
// (no SA_RESTORER is installed -- see numbers_power.h's own comment),
// which must reach signotify()'s own eventual return path completely
// unclobbered for the "noted(NCONT) -> signotify() returns normally"
// case to resume the interrupted context correctly (matching what
// arm's plain tail-branch achieves by simply never touching LR at
// all). Since BL itself unavoidably overwrites LR (to point at the
// instruction right after it, for a real subroutine return), save the
// kernel's original LR first and restore it before returning --
// net effect identical to a tail-branch, built from primitives (BL,
// register MOVW, RETURN-via-LR) already confirmed working elsewhere
// in this tree.
TEXT sigentry(SB), $0
	MOVW	LR, R4
	BL	signotify(SB)
	MOVW	R4, LR
	RETURN
