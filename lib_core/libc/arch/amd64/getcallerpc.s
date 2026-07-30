// getcallerpc(v) -- see arch/386/getcallerpc.s's identical comment;
// same trick, widened to this arch's 8-byte pointers/return addresses
// (MOVQ, -8 not -4). Not verified via 6c -S (unlike every register-
// vs-stack-argument fact elsewhere in this tree) since the return
// value is only ever used for setmalloctag()'s own no-op debug
// bookkeeping (port/minimal_malloc.c) -- nothing currently reads it,
// so a wrong value here can't affect any program's actual output;
// revisit with real -S verification before relying on it for anything
// that does.
//
// Also: only ever built/run under GOOS=linux so far, same "assumed
// but not proven" OS-independence caveat as arch/386/getcallerpc.s's
// identical comment (rt0.s's own darwin/amd64 exception is the
// precedent for why this arch/$cputype/ location isn't automatically
// a safe bet across every GOOS).
TEXT getcallerpc+0(SB), $0
	MOVQ	v+0(FP), AX
	MOVQ	-8(AX), AX
	RET
