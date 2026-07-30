// Raw Plan9 (GOOS=plan9) syscall stubs for arm. Unlike Linux/Darwin's
// generic _syscall6 trampoline (lib_core/libc/syscall/os/linux/
// svc_arm.s), Plan9's own syscall convention needs no shared
// marshalling helper at all: the kernel reads syscall arguments
// starting 4 bytes past the stack pointer at trap time (see
// docs/claude_notes/notes_abi_plan9.txt), which is exactly where this
// compiler's own calling convention already puts a function's own
// arguments -- confirmed against linux/arm/svc_arm.s's own empirically-
// verified fact ("only the *first* named parameter (num) arrives in a
// register (R0), every *other* argument packs at FP+4, FP+8, ...";
// this is an AAPCS-family calling-convention fact, not Linux-specific,
// so it applies here unchanged). So each syscall here just spills its
// own first argument from R0 to its home slot 0(FP) (making the full
// argument list contiguous on the stack, matching what's already
// there from FP+4 onward), puts the syscall number in R0, and traps --
// no generic trampoline needed, one function per syscall.
//
// Adapted from ~/principia-softwarica/lib_core/libc/9syscall/mkfile's
// per-syscall stub generator (real, working Plan9 syscall stubs for a
// project this one is meant to interoperate with -- see
// lib_core/libc/syscall/os/plan9/sys.h's own "coupling: principia"
// comment), not written from scratch -- but goken's own 5c wasn't
// independently re-verified via -S probing before this file existed
// (no genuine GOOS=plan9 target existed yet); the R0-in-first-arg /
// FP+4-onward-for-the-rest facts above ARE independently confirmed
// (they're the same facts linux/arm/svc_arm.s already established).
#include "sys.h"

// exits(char *msg): Plan9's real process-exit syscall, taking a status
// *string* (nil/empty means success, any other string means failure
// with that string as the reason) -- not POSIX's int exit code. See
// syscall_plan9_arm.h's exit(int) for the POSIX-style adapter goken's
// other GOOS ports already provide (include/os/proc.h's own extern
// void exit(int);).
TEXT exits(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$EXITS, R0
	SWI	$0
	RET

TEXT open(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$OPEN, R0
	SWI	$0
	RET

TEXT close(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$CLOSE, R0
	SWI	$0
	RET

TEXT pread(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$PREAD, R0
	SWI	$0
	RET

TEXT pwrite(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$PWRITE, R0
	SWI	$0
	RET

// seek's raw syscall return doesn't fit the usual "result in R0"
// shape: a vlong (8-byte) C return value is returned via a hidden
// pointer this compiler passes as the function's own first argument
// (same register-spill dance as every other stub above, since as far
// as the calling convention is concerned that hidden pointer just IS
// this function's first argument) -- the kernel's real sysseek()
// (principia's kernel/files/sysfile.c) plainly `return 0`s on success
// and only ever produces -1 via Plan9's general error-note convention
// (any syscall that calls error() gets its return register forced to
// -1, regardless of which specific error), so write-back happens
// whenever the raw result is NOT -1. principia's own generator script
// (9syscall/mkfile) writes back on the *opposite* condition (R0==-1) --
// concluded to be its own latent, seemingly-never-exercised bug (see
// docs/claude_notes/notes_abi_plan9.txt's own "only PWRITE/EXITS ever
// exercised" caveat, which covers principia's generator too, not just
// goken); this version was written the way sysseek()'s own C source
// reads, then verified against a real seek() call under 5i, not
// assumed either way -- see that same notes file for the verification
// record.
TEXT seek(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$SEEK, R0
	SWI	$0
	MOVW	$-1, R2
	CMP	R2, R0
	BEQ	seekdone
	MOVW	0(FP), R1
	MOVW	R0, 0(R1)
	MOVW	R0, 4(R1)
seekdone:
	RET
