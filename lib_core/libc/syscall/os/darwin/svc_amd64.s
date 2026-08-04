#include "numbers_amd64.h"

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
// XNU's error convention (carry flag set on return, AX holds the
// positive errno) differs from Linux's (negative errno already packed
// into the return value, no flag to check) -- this WAS unhandled here
// (see git history), harmless for write/exit since nothing ever
// inspected their return value, but open()/read() are exactly the
// "syscall a caller actually needs to detect failure from" this
// comment used to warn about. JCC/NEGQ below normalize XNU's convention
// to Linux's: success falls through with AX untouched, failure negates
// AX so callers everywhere can use the one `ret < 0` => -errno check.
// This is the minimum fix needed to make _syscall6's return value mean
// anything at all on error, hence living here in the raw trampoline
// rather than in os/darwin/'s Plan9-API-shape glue (see that
// directory's own header comment for the syscall/ vs os/ split).
// Not yet exercised against a real XNU failure (e.g. open() on a
// missing path) -- this host has no macOS available; verify on real
// hardware before trusting it fully (see docs/claude_notes/
// notes_libc_selfhost.txt).
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
	JCC	ok
	NEGQ	AX
ok:
	RET

// claude: _syscall6v -- byte-identical to _syscall6 above (including
// the XNU carry-flag normalization), a separate symbol purely so its C
// prototype (syscall_darwin_amd64.h) can declare a `vlong` return
// instead of `long`, for the one caller (lseek) that actually needs
// the kernel's full 64-bit result. See scripts/mksyscall.sh's header
// comment for why this is a second trampoline rather than widening
// _syscall6 itself.
TEXT _syscall6v+0(SB), $0
	MOVQ	num+0(FP), AX
	ADDQ	$0x2000000, AX
	MOVQ	a1+8(FP), DI
	MOVQ	a2+16(FP), SI
	MOVQ	a3+24(FP), DX
	MOVQ	a4+32(FP), R10
	MOVQ	a5+40(FP), R8
	MOVQ	a6+48(FP), R9
	SYSCALL
	JCC	okv
	NEGQ	AX
okv:
	RET

// claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
// fork()/pipe() cannot go through _syscall6 above at all, even though
// both are otherwise argument-light: XNU's raw fork(2) and pipe(2) use
// the classic BSD dual-register return convention, which a single
// `long` result can't express. Confirmed both need this (not merely
// suspected) by numbers_amd64.h's own citation of bsd/kern/
// syscalls.master: both are marked NO_SYSCALL_STUB, meaning even
// Apple's own libSystem hand-writes custom assembly for them instead
// of the generic per-syscall stub every other BSD call gets.
//
// fork(2): on success BOTH parent and child come back with AX holding
// the PARENT's pid; DX distinguishes them (0 in the parent, nonzero --
// in practice 1 -- in the child). The public POSIX/Plan9 contract (0
// in the child, the child's real pid in the parent) only holds once
// this stub zeroes AX itself when DX is nonzero. port/fork.c (the one
// Plan9-shaped bridge every GOOS shares) calls this exactly like any
// other _sysfork() and never sees DX at all.
TEXT _sysfork+0(SB), $0
	MOVQ	$(0x2000000+SYS_fork), AX
	SYSCALL
	JCC	forkok
	NEGQ	AX
	RET
forkok:
	CMPQ	DX, $0
	JEQ	forkdone
	XORQ	AX, AX
forkdone:
	RET

// pipe(2) ("{ int pipe(void); }" in syscalls.master -- genuinely zero
// arguments, not even the pointer POSIX's own prototype implies): on
// success AX holds the READ fd and DX holds the WRITE fd, both via
// registers, nothing written to memory by the kernel at all.
// _syspipe(int *fd) (the name port/pipe.c calls, same as every other
// GOOS) takes the ordinary pointer argument and writes the two
// registers through it here -- the one place in this file that turns
// a register-pair kernel result into the normal pointer-out shape
// every caller above this layer expects.
TEXT _syspipe+0(SB), $0
	MOVQ	fd+0(FP), CX
	MOVQ	$(0x2000000+SYS_pipe), AX
	SYSCALL
	JCC	pipeok
	NEGQ	AX
	RET
pipeok:
	MOVL	AX, 0(CX)
	MOVL	DX, 4(CX)
	XORQ	AX, AX
	RET
