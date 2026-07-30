// The only raw syscall entry point for linux/riscv64 (rv64): loads up
// to 6 args into the registers the kernel's ECALL handler expects
// (a7=R17=number, a0-a5=R10-R15=args) and traps. Every OS/arch's
// syscall wrappers (see syscall/os/$OS/) are generated thin C
// functions calling this, so this is the one place that ever needs
// hand-written assembly for this arch.
//
// note on the FP offsets below: like riscv/svc.s (rv32), only the
// *first* named parameter (num) arrives in a register (R8), read
// directly since this hand-written function gets no compiler prologue
// spill. Unlike rv32 though, a1..a6 here are declared `vlong` (8
// bytes), not `long` (4 bytes) -- see
// syscall/os/linux/syscall_linux_riscv64.h's own comment for why: a
// plain `long` stays 4 bytes even on this 64-bit arch, so a syscall
// argument that's actually a pointer (e.g. write()'s buf) would get
// silently truncated passing through a `long`-typed a1. Confirmed via
// ic -S that a `vlong` a1 lands at FP+8 (not FP+4 -- the 8-byte type
// forces the same alignment rounding include/arch/riscv64/u.h's
// va_arg needs), with each subsequent vlong arg spaced 8 bytes apart
// (FP+16, +24, +32, +40, +48) since every remaining argument is also
// 8 bytes wide -- read with MOV (not MOVW) accordingly.
//
// return value: like every arch here, the caller expects this
// function's own `long` return in R8, not wherever ECALL happens to
// leave it (R10/a0) -- an explicit copy is needed.
TEXT _syscall6+0(SB), $0
	MOVW	R8, R17          // a7 = syscall number (num arrives in R8)
	MOV	a1+8(FP), R10    // a0
	MOV	a2+16(FP), R11   // a1
	MOV	a3+24(FP), R12   // a2
	MOV	a4+32(FP), R13   // a3
	MOV	a5+40(FP), R14   // a4
	MOV	a6+48(FP), R15   // a5
	ECALL
	MOVW	R10, R8          // return value: a0 -> R8
	RET

// claude: _syscall6v -- same trap sequence as _syscall6 above, a
// separate symbol so its C prototype (syscall_linux_riscv64.h) can
// declare a `vlong` return instead of `long`, for the one caller
// (lseek) that actually needs the kernel's full 64-bit result. The one
// real difference from _syscall6: the return-value copy uses MOV (full
// 64-bit a0 -> R8), not MOVW (32-bit, would drop the upper half a
// `vlong`-returning caller expects to find in R8). See
// scripts/mksyscall.sh's header comment for why this is a second
// trampoline rather than widening _syscall6 itself.
TEXT _syscall6v+0(SB), $0
	MOVW	R8, R17          // a7 = syscall number (num arrives in R8)
	MOV	a1+8(FP), R10    // a0
	MOV	a2+16(FP), R11   // a1
	MOV	a3+24(FP), R12   // a2
	MOV	a4+32(FP), R13   // a3
	MOV	a5+40(FP), R14   // a4
	MOV	a6+48(FP), R15   // a5
	ECALL
	MOV	R10, R8          // full 64-bit return value: a0 -> R8
	RET
