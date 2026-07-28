// The only raw syscall entry point for darwin/arm64: loads up to 6 args
// into the registers XNU's SVC handler expects and traps. Every
// OS/arch's syscall wrappers (see syscall/os/$OS/) are generated thin C
// functions calling this, so this is the one place that ever needs
// hand-written assembly for this (arch, OS) pair.
//
// Two concrete differences from linux/arm64's svc_arm64.s, both
// confirmed empirically against real macOS execution while bringing up
// 7l's -H6 Mach-O target (see docs/claude_notes/notes_exec_macho.txt
// and tests/s/mini/hello_macos_arm64.s, which predates this file and
// is where these two facts were first nailed down): the syscall number
// goes in R16, not R8, and the trap immediate is $0x80, not $0 (SVC $0
// on Darwin is a Mach trap, a completely different call table). Every
// other detail -- args in R0-R5, num arriving in the caller's R0
// (unspilled, since this is hand-written assembly with no
// compiler-generated prologue -- see linux/arm64's svc_arm64.s for the
// full FP-vs-register reasoning, which is an AAPCS64 calling-convention
// fact and so applies identically regardless of OS) -- is unchanged.
//
// Not handled here: XNU's error convention (carry flag set + errno in
// x0) differs from Linux's (negative errno packed into the return
// value). Same limitation already exists for linux/arm64's svc_arm64.s
// -- neither wrapper checks for syscall failure, which is fine for
// write/exit in a minimal libc with no error-handling callers yet, but
// worth fixing before adding a syscall a caller actually needs to
// detect failure from.
TEXT _syscall6+0(SB), $0
	MOV	R0, R16
	MOV	a1+8(FP), R0
	MOV	a2+16(FP), R1
	MOV	a3+24(FP), R2
	MOV	a4+32(FP), R3
	MOV	a5+40(FP), R4
	MOV	a6+48(FP), R5
	SVC	$0x80
	RETURN
