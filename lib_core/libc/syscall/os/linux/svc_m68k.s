// The only raw syscall entry point for linux/m68k: loads args into
// the registers the kernel's `trap #0` handler expects (d0=number,
// d1-d5=args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so
// this is the one place that ever needs hand-written assembly for
// this arch.
//
// note on the FP offsets below: like sparc's own svc_sparc.s, 2c
// passes every argument to this function on the stack (no REGARG at
// all -- see docs/claude_notes/notes_arch_m68k.txt) -- confirmed
// against '2c -S' on this exact 7-argument signature: num at 0(FP),
// a1 at 4(FP), ..., a6 at 24(FP), the same 4-bytes-per-slot pattern
// xwrite_m68k.s/write() (tests/c/mini2, tests/c/vlong, tests/c/float)
// already established for their own stack args.
//
// claude: a6 is deliberately never read here -- real m68k Linux
// syscall entry only has 5 argument registers (d1-d5, since d0 is the
// syscall number), and unlike svc_mips.s's own a5/a6 (which DOES need
// a stack-based extension, confirmed against a real kernel source
// checkout, see that file's comment), no syscall this tree currently
// declares (syscall_linux_m68k.decl) needs more than 5 real arguments
// -- _sysrenameat2 is the widest at 5. If a future syscall needs a
// true 6th argument, this will need the same stack-relative extension
// mips's own svc_mips.s has (unverified here: no real kernel-ABI
// reference for m68k's own 6-argument syscall stack convention was
// found while bringing this up).
//
// No return-value copy needed: real m68k Linux syscalls return their
// result directly in d0 (this arch's REGRET too, confirmed by
// xwrite_m68k.s's own comment) using the STANDARD negative-errno
// convention every arch but o32-mips/sparc here uses -- no carry-bit
// or out-of-band error flag to convert, unlike svc_sparc.s.
TEXT _syscall6+0(SB), $0
	MOVL	num+0(FP), R0
	MOVL	a1+4(FP), R1
	MOVL	a2+8(FP), R2
	MOVL	a3+12(FP), R3
	MOVL	a4+16(FP), R4
	MOVL	a5+20(FP), R5
	TRAP	$0
	RTS
