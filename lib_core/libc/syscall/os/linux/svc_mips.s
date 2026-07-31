// The only raw syscall entry point for linux/mips (o32): loads args
// into the registers the kernel's SYSCALL handler expects (R2=number,
// R4-R7=first 4 args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this arch.
//
// note: real o32 Linux syscalls with a 5th/6th argument need those
// passed on the caller's stack (not a plain register), a wrinkle
// unique to mips among the archs handled so far. Not implemented here
// since neither syscall declared in syscall_linux_mips.decl (write,
// exit) needs more than 3 args -- if a future syscall does, this is
// the place to add it.
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
TEXT _syscall6+0(SB), $0
	MOVW	R1, R2
	MOVW	a1+4(FP), R4
	MOVW	a2+8(FP), R5
	MOVW	a3+12(FP), R6
	MOVW	a4+16(FP), R7
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
