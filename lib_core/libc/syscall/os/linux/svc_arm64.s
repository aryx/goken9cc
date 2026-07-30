// The only raw syscall entry point for linux/arm64: loads up to 6 args
// into the registers the kernel's SVC handler expects (x8=number,
// x0-x5=args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this arch.
//
// note on the FP offsets below: only the *first* named parameter (num)
// arrives in a register (R0) -- every later one is stack-passed by the
// caller, landing at FP+8, FP+16, ... in the callee (confirmed against
// real 7c -S output: a compiler-generated callee's own prologue spills
// its R0 parameter to FP+0 for uniform FP-relative access, but this is
// hand-written assembly with no such prologue, so num must be read
// directly from R0 -- reading FP+0 here would be garbage). This is also
// why tests/c/mini2/linux_arm64.s's write() stub never loads fd from
// FP+0(it arrives in R0 and is also what the syscall itself expects
// there for write, so it needs no move at all) -- not a bug, just an
// unstated reliance on this same convention.
TEXT _syscall6+0(SB), $0
	MOV	R0, R8
	MOV	a1+8(FP), R0
	MOV	a2+16(FP), R1
	MOV	a3+24(FP), R2
	MOV	a4+32(FP), R3
	MOV	a5+40(FP), R4
	MOV	a6+48(FP), R5
	SVC	$0
	RETURN

// claude: _syscall6v -- byte-identical to _syscall6 above, a separate
// symbol purely so its C prototype (syscall_linux_arm64.h) can declare
// a `vlong` return instead of `long`, for the one caller (lseek) that
// actually needs the kernel's full 64-bit result. See
// scripts/mksyscall.sh's header comment for why this is a second
// trampoline rather than widening _syscall6 itself.
TEXT _syscall6v+0(SB), $0
	MOV	R0, R8
	MOV	a1+8(FP), R0
	MOV	a2+16(FP), R1
	MOV	a3+24(FP), R2
	MOV	a4+32(FP), R3
	MOV	a5+40(FP), R4
	MOV	a6+48(FP), R5
	SVC	$0
	RETURN
