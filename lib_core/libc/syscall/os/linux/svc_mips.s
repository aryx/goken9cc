// The only raw syscall entry point for linux/mips (o32): loads args
// into the registers the kernel's SYSCALL handler expects (R2=number,
// R4-R7=first 4 args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this arch.
//
// note on the FP offsets below: like arm64, only the *first* named
// parameter (num) arrives in a register (R1) -- confirmed against
// vc -S output, a compiler-generated callee only spills it to its home
// slot (arg+0(FP)) if its address is actually taken, which this
// hand-written function never does, so num must come from R1 directly.
// Unlike arm64 (uniform 8-byte slots) though, every *other* argument
// here is packed at its natural 4-byte width with no padding, and
// critically the first argument's (unused) home slot still reserves
// FP+0 -- so a1 starts at FP+4, not FP+0 (confirmed against
// tests/c/mini2/linux_mips.s's write() stub, which already relies on
// this same offset for its own buf+4(FP)).
//
// claude: a5/a6 (the 5th/6th real syscall arguments -- renameat2's
// `flags` is the first live user of a5, see numbers_mips.h's own
// comment) do NOT go in a register at all: real o32 Linux syscalls
// with more than 4 arguments read the extras off the CALLER's stack,
// at $sp+16 and $sp+20 *at the moment of the SYSCALL trap* -- a kernel
// convention with nothing to do with this compiler's own calling
// convention for _syscall6 itself. This function has frame size $0
// (no prologue stack adjustment), so R29 here IS the raw hardware SP
// the kernel will read -- and it already holds a5/a6's OWN incoming
// values at FP+20/FP+24 by coincidence of this compiler's ABI (every
// argument after num is stack-passed, num's own wasted home slot at
// FP+0 pushes everything else out by 4 bytes -- see above), but not at
// the OFFSETS the kernel wants (+16/+20 relative to the same SP, not
// +20/+24). So both still have to be read into a register and
// re-stored at the kernel's own expected offset before SYSCALL, not
// merely left where the caller happened to put them.
//
// Safe to do unconditionally, even for a syscall that uses none of the
// six argument slots (e.g. fork()): scripts/mksyscall.sh always
// generates a literal 7-argument call to _syscall6 (unused trailing
// slots padded with 0, e.g. "_syscall6(SYS_fork, 0, 0, 0, 0, 0, 0)"),
// so the caller always reserves the full 28-byte argument area this
// needs, regardless of which real syscall is being made.
//
// Found (a5 dropped, silently) via docs/claude_notes/plan_syscalls.txt's
// own Tier 4 process-control work, which cross-checked wait4's 4-arg
// fit against this file and noticed _sysrenameat2 (Tier 3.5, added
// earlier) already needed a 5th -- tracked in todo.org until fixed
// here. a6 has no real caller yet (no six-argument mips syscall in
// this tree), so its own half of this fix is unexercised by anything
// beyond eyeballing the symmetry with a5's -- verify against a real
// caller before trusting it blindly.
TEXT _syscall6+0(SB), $0
	MOVW	R1, R2
	MOVW	a1+4(FP), R4
	MOVW	a2+8(FP), R5
	MOVW	a3+12(FP), R6
	MOVW	a4+16(FP), R7
	MOVW	a5+20(FP), R8
	MOVW	R8, 16(R29)
	MOVW	a6+24(FP), R9
	MOVW	R9, 20(R29)
	SYSCALL
	// Real Linux/mips o32 syscall convention returns the result in
	// $v0 (R2), same register the syscall number went in on -- but
	// this compiler's own calling convention returns a function's
	// `long` result via R1 (confirmed empirically: without this move,
	// open()/read() came back holding the raw syscall *number*
	// (SYS_open/SYS_read) unchanged, i.e. RET was returning R1's
	// stale value from the MOVW above, never touched again after
	// function entry -- write()/exit() never surfaced this since
	// nothing ever inspected their return value). Move the kernel's
	// real result from R2 into R1 before RET so callers actually see
	// it.
	MOVW	R2, R1
	// claude: ...and then convert o32's error convention into the
	// negative-errno one every other arch's _syscall6 already returns,
	// because mips is the odd one out here. On 386/amd64/arm/arm64/
	// riscv/riscv64 a failing syscall returns -errno directly in the
	// result register, so a caller's `if(fd < 0)` just works. o32 mips
	// instead reports failure out-of-band in $a3 (R7): 0 on success, 1
	// on error, with $v0 holding a POSITIVE errno in the error case. So
	// without this, open() on a missing file returned +2 (ENOENT), which
	// is >= 0 -- i.e. every error check in libc silently read a failure
	// as a valid small fd on this arch alone.
	//
	// Found by tests/c/hello_libc/dir.c, the first test in this tree to
	// check a *failing* syscall's return value on mips (it requires
	// open() to fail after remove() deletes the file); hello.c/io.c only
	// ever make calls that succeed, which is the same reason the MOVW
	// R2,R1 above went unnoticed for so long. Verified against a
	// qemu-mips -strace showing the kernel itself returning the right
	// answer (unlink = 0, then open = -1 errno=2) while the C caller saw
	// +2.
	//
	// R0 is mips's hardwired zero register (include/obj/v.out.h's
	// REGZERO), and this assembler's SUBU is "dst = src1 - src2" written
	// `SUBU src2, src1, dst` (see syscall/os/plan9/svc_mips.s's own
	// SUBU $-1, R1, R3), so this negates $v0 into the return register.
	// BEQ takes a single register operand (branch if zero) in this
	// assembler -- again as in the plan9 svc_mips.s.
	BEQ	R7, syscallok
	SUBU	R2, R0, R1
syscallok:
	RET
