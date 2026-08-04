#include "numbers_arm64.h"

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
// XNU's error convention (carry flag set on return, R0 holds the
// positive errno) differs from Linux's (negative errno already packed
// into the return value, no flag to check) -- see svc_amd64.s's
// identical BCC/NEG normalization and its comment on why this belongs
// in the raw trampoline rather than os/darwin/'s glue: it's the
// minimum fix needed to make _syscall6's return value mean anything at
// all on error. Not yet exercised against a real XNU failure -- this
// host has no macOS available; verify on real hardware before trusting
// it fully (see docs/claude_notes/notes_libc_selfhost.txt).
TEXT _syscall6+0(SB), $0
	MOV	R0, R16
	MOV	a1+8(FP), R0
	MOV	a2+16(FP), R1
	MOV	a3+24(FP), R2
	MOV	a4+32(FP), R3
	MOV	a5+40(FP), R4
	MOV	a6+48(FP), R5
	SVC	$0x80
	BCC	ok
	NEG	R0, R0
ok:
	RETURN

// claude: _syscall6v -- byte-identical to _syscall6 above (including
// the XNU carry-flag normalization), a separate symbol purely so its C
// prototype (syscall_darwin_arm64.h) can declare a `vlong` return
// instead of `long`, for the one caller (lseek) that actually needs
// the kernel's full 64-bit result. See scripts/mksyscall.sh's header
// comment for why this is a second trampoline rather than widening
// _syscall6 itself.
TEXT _syscall6v+0(SB), $0
	MOV	R0, R16
	MOV	a1+8(FP), R0
	MOV	a2+16(FP), R1
	MOV	a3+24(FP), R2
	MOV	a4+32(FP), R3
	MOV	a5+40(FP), R4
	MOV	a6+48(FP), R5
	SVC	$0x80
	BCC	okv
	NEG	R0, R0
okv:
	RETURN

// claude: Tier 4 process control -- see svc_amd64.s's identical
// comment on why fork()/pipe() need dedicated asm here instead of the
// generic _syscall6 trampoline (XNU's classic BSD dual-register return
// convention -- isChild in X1 for fork, the write fd in X1 for pipe;
// both marked NO_SYSCALL_STUB in bsd/kern/syscalls.master, confirming
// even Apple's own libSystem hand-writes custom assembly for both).
// Same register story as _syscall6's own num-in-R0 above: the first
// (and here, only) argument always arrives unspilled in R0 -- fork()
// takes no arguments at all, and pipe's single fd* argument is exactly
// that first-argument case, so it must be saved to a callee-safe
// register (R2) before SVC overwrites R0 with the kernel's own result.
TEXT _sysfork+0(SB), $0
	MOV	$SYS_fork, R16
	SVC	$0x80
	BCC	forkok
	NEG	R0, R0
	RETURN
forkok:
	CBZ	R1, forkdone
	MOV	$0, R0
forkdone:
	RETURN

// pipe(2) ("{ int pipe(void); }" in syscalls.master -- genuinely zero
// arguments): on success R0 holds the READ fd and R1 holds the WRITE
// fd, both via registers, nothing written to memory by the kernel at
// all. _syspipe(int *fd) (the name port/pipe.c calls, same as every
// other GOOS) takes the ordinary pointer argument and writes the two
// registers through it here.
TEXT _syspipe+0(SB), $0
	MOV	R0, R2
	MOV	$SYS_pipe, R16
	SVC	$0x80
	BCC	pipeok
	NEG	R0, R0
	RETURN
pipeok:
	MOVW	R0, 0(R2)
	MOVW	R1, 4(R2)
	MOV	$0, R0
	RETURN
