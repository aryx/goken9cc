// The only raw syscall entry point for linux/sparc: loads up to 6 args
// into the registers the kernel's `ta 0x10` handler expects (g1=R1=
// number, o0-o5=R8-R13=args) and traps. Every OS/arch's syscall
// wrappers (see syscall/os/$OS/) are generated thin C functions calling
// this, so this is the one place that ever needs hand-written assembly
// for this arch.
//
// note on the FP offsets below: kc passes this function's own FIRST
// argument (num) in R7 (REGARG), and every argument after that on the
// stack -- confirmed against 'kc -S' on this exact 7-argument
// signature: num arrives in R7, a1 at a1+4(FP), a2 at a2+8(FP), ...,
// a6 at a6+24(FP), the same 4-bytes-per-slot pattern already
// established by xwrite_sparc.s/write() (tests/c/mini2, tests/c/vlong,
// tests/c/float)'s own count+N(FP) reads.
//
// R1 (REGSP, kc's own logical stack pointer -- see docs/claude_notes/
// notes_arch_sparc.txt) collides with the real hardware %g1, the
// syscall-number register -- the same trap xwrite_sparc.s already
// documents and works around. Read every FP-relative argument BEFORE
// clobbering R1 with the syscall number, then save/restore it around
// the trap exactly the same way.
//
// Linux SPARC32 syscall ABI signals errors through the carry (icc.C)
// condition bit, NOT a negative return value the way every other arch
// here does (see tests/s/exit/exit_linux_sparc.s's own comment) -- so
// unlike svc_386.s/svc_amd64.s/svc_arm.s (whose raw trap result is
// already the -errno every caller in this tree expects), this
// function has real conversion work to do: BCC (branch if carry
// clear) right after the trap, before anything else can disturb the
// condition codes, and negate the raw (positive) errno on the error
// path -- same shape as svc_mips.s's own out-of-band ($a3) check, a
// different register/flag but the identical underlying reason (o32
// mips and SPARC both predate the "just return -errno" convention
// every later Linux port standardized on).
//
// claude: the R1 restore is duplicated on BOTH the taken and
// fall-through paths below, NOT shared via BCC's own branch-delay
// slot -- a real, confirmed bug in an earlier version of this file
// that put a single "MOVW R14,R1" textually right after the BCC,
// assuming (as real bare-hardware SPARC assembly would guarantee)
// that the next instruction in program order IS the delay slot and
// therefore always executes. kl's own scheduler does not honor that:
// 'kl -a' on the linked binary showed it had synthesized a genuine
// NOP ("ORN R0,R0") as BCC's actual delay slot and moved the intended
// restore to be executed ONLY on the fall-through (error) path,
// leaving R1 holding the raw syscall number (never restored to
// REGSP) on the taken (success) path. Every later instruction in the
// CALLER (write(), back up in its own auto-generated epilogue) then
// dereferenced that garbage R1 as if it were still a valid stack
// pointer -- a segfault at a tiny address (the syscall number itself,
// e.g. 0x4 for SYS_write), NOT at 0xfffffff8 the way an entirely
// uninitialized R1 faults (see notes_arch_sparc.txt's tests/c/mini
// section) -- a useful fingerprint for telling the two bugs apart.
// Lesson: never rely on this compiler suite's assembler placing a
// hand-written instruction into a branch's delay slot just because it
// appears next in the source; duplicate work explicitly on every
// path instead, or confirm the real placement with 'kl -a' first.
TEXT _syscall6+0(SB), $0
	MOVW	a1+4(FP), R8		// -> o0
	MOVW	a2+8(FP), R9		// -> o1
	MOVW	a3+12(FP), R10		// -> o2
	MOVW	a4+16(FP), R11		// -> o3
	MOVW	a5+20(FP), R12		// -> o4
	MOVW	a6+24(FP), R13		// -> o5
	MOVW	R1, R14			// save REGSP before clobbering it below
	MOVW	R7, R1			// num (arrives in R7) -> g1
	MOVW	$0x10, R7		// scratch reg holding the trap number
	TA	R7
	BCC	sysok
	MOVW	R14, R1			// fall-through (error) path: restore REGSP
	SUB	R0, R8, R7		// error: R7 = 0 - errno (o0 has the raw positive errno)
	RETURN
sysok:
	MOVW	R14, R1			// taken (success) path: restore REGSP
	MOVW	R8, R7			// success: syscall return (o0) -> REGRET
	RETURN
