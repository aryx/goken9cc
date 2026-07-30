// getcallerpc(v) -- unlike 386/amd64 (arch/386/getcallerpc.s), this
// arch's own calling convention passes the first argument in a
// register (R0), not the stack (see e.g. syscall/os/linux/svc_arm.s's
// sibling comments elsewhere in this tree) -- so this doesn't use its
// `v` argument at all. Adapted verbatim from ~/principia-softwarica/
// lib_core/libc/arm/getcallerpc.s (a real, working reference), whose
// approach -- reading 0(R13)/SP directly -- relies on port/strdup.c's
// own call site ("getcallerpc(&s)", from inside strdup(), a non-leaf
// function) having already saved its incoming LR to the stack before
// calling anything else. Not verified via 5c -S this session -- see
// arch/amd64/getcallerpc.s's identical caveat on why a wrong value
// here is low-risk: nothing currently reads the result.
//
// Also: only ever built/run under GOOS=linux so far, same "assumed
// but not proven" OS-independence caveat as arch/386/getcallerpc.s's
// identical comment -- and arm is exactly the arch this project's own
// syscall/os/plan9/svc_arm.s work landed for a *third* GOOS this same
// week, so if anything is going to expose a GOOS-specific difference
// here first, it's this one.
TEXT getcallerpc+0(SB), $0
	MOVW	0(R13), R0
	RET
