// The only raw syscall entry point for darwin/amd64: loads up to 6 args
// into the registers XNU's SYSCALL handler expects, adds the 0x2000000
// "BSD class" prefix Darwin's amd64 syscall convention bakes into the
// number itself, and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this
// (arch, OS) pair.
//
// The 0x2000000 prefix is the one concrete difference from
// linux/amd64's svc_amd64.s, confirmed empirically against real macOS
// execution while bringing up 6l's -H6 Mach-O target (see
// docs/claude_notes/notes_exec_macho.txt and
// tests/s/mini/hello_macos_amd64.s, which predates this file and is
// where this fact was first nailed down; arm64 has no such prefix, see
// svc_arm64.s in this same directory -- a genuine cross-arch asymmetry
// in how Darwin encodes the same BSD syscall numbers, not a copy-paste
// inconsistency). Everything else is unchanged from linux/amd64's
// version: no argument (not even num) arrives in a register -- this is
// an AMD64 SysV ABI fact, not an OS-specific one, so it applies
// identically regardless of GOOS -- the caller always writes every
// argument to the stack before CALL, letting this hand-written
// function read num+0(FP) directly with no register-vs-FP special
// case (see linux/amd64's svc_amd64.s for the full reasoning).
//
// Not handled here: XNU's error convention (carry flag set + errno in
// AX) differs from Linux's (negative errno packed into the return
// value). Same limitation already exists for linux/amd64's and
// darwin/arm64's svc*.s -- neither wrapper checks for syscall failure,
// which is fine for write/exit in a minimal libc with no
// error-handling callers yet, but worth fixing before adding a
// syscall a caller actually needs to detect failure from.
TEXT _syscall6+0(SB), $0
	MOVQ	num+0(FP), AX
	ADDQ	$0x2000000, AX
	MOVQ	a1+8(FP), DI
	MOVQ	a2+16(FP), SI
	MOVQ	a3+24(FP), DX
	MOVQ	a4+32(FP), R10
	MOVQ	a5+40(FP), R8
	MOVQ	a6+48(FP), R9
	SYSCALL
	RET
