// Raw Plan9 (GOOS=plan9) syscall stubs for arm. Unlike Linux/Darwin's
// generic _syscall6 trampoline (lib_core/libc/syscall/os/linux/
// svc_arm.s), Plan9's own syscall convention needs no shared
// marshalling helper at all: the kernel reads syscall arguments
// starting 4 bytes past the stack pointer at trap time (see
// docs/claude_notes/notes_abi_plan9.txt), which is exactly where this
// compiler's own calling convention already puts a function's own
// arguments -- confirmed against linux/arm/svc_arm.s's own empirically-
// verified fact ("only the *first* named parameter (num) arrives in a
// register (R0), every *other* argument packs at FP+4, FP+8, ...";
// this is an AAPCS-family calling-convention fact, not Linux-specific,
// so it applies here unchanged). So each syscall here just spills its
// own first argument from R0 to its home slot 0(FP) (making the full
// argument list contiguous on the stack, matching what's already
// there from FP+4 onward), puts the syscall number in R0, and traps --
// no generic trampoline needed, one function per syscall.
//
// Adapted from ~/principia-softwarica/lib_core/libc/9syscall/mkfile's
// per-syscall stub generator (real, working Plan9 syscall stubs for a
// project this one is meant to interoperate with -- see
// lib_core/libc/syscall/os/plan9/sys.h's own "coupling: principia"
// comment), not written from scratch -- but goken's own 5c wasn't
// independently re-verified via -S probing before this file existed
// (no genuine GOOS=plan9 target existed yet); the R0-in-first-arg /
// FP+4-onward-for-the-rest facts above ARE independently confirmed
// (they're the same facts linux/arm/svc_arm.s already established).
#include "sys.h"

// exits(char *msg): Plan9's real process-exit syscall, taking a status
// *string* (nil/empty means success, any other string means failure
// with that string as the reason) -- not POSIX's int exit code. See
// syscall_plan9_arm.h's exit(int) for the POSIX-style adapter goken's
// other GOOS ports already provide (include/os/proc.h's own extern
// void exit(int);).
TEXT exits(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$EXITS, R0
	SWI	$0
	RET

TEXT open(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$OPEN, R0
	SWI	$0
	RET

TEXT close(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$CLOSE, R0
	SWI	$0
	RET

// create/remove/chdir: the clearest illustration of why GOOS=plan9
// needs no os/plan9/ glue layer (see lib_core/libc/os/plan9/open.c's
// "intentionally empty" comment). On Linux and Darwin, remove() and
// chdir() at least map onto identically-shaped POSIX syscalls, but
// create() genuinely has to be *built* out of open(O_CREAT|O_TRUNC)
// with the Plan9 mode bits translated into O_* flags, and DMDIR
// rejected for want of a mkdir syscall (os/linux/open.c). Here all
// three are the real kernel calls, with the real signatures
// include/os/dir.h already declares -- create(char*, int, ulong)
// included, DMDIR and all.
TEXT create(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$CREATE, R0
	SWI	$0
	RET

TEXT remove(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$REMOVE, R0
	SWI	$0
	RET

TEXT chdir(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$CHDIR, R0
	SWI	$0
	RET

TEXT pread(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$PREAD, R0
	SWI	$0
	RET

TEXT pwrite(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$PWRITE, R0
	SWI	$0
	RET

// seek's raw syscall return doesn't fit the usual "result in R0"
// shape: a vlong (8-byte) C return value is returned via a hidden
// pointer this compiler passes as the function's own first argument
// (same register-spill dance as every other stub above, since as far
// as the calling convention is concerned that hidden pointer just IS
// this function's first argument).
//
// THE KERNEL ITSELF FILLS THAT POINTER IN on success -- principia's
// kernel/files/sysfile.c sysseek() validates arg[0], calls sseek(arg)
// (which writes the resulting 8-byte offset through it), and then
// plainly `return 0`s, so the return REGISTER carries no offset at all.
// machines/5i/syscall_posix.c's sysseek() models exactly this, ending
// in putmem_v(retp, v). What the register does carry is Plan9's general
// error-note convention: any syscall that calls error() gets its return
// register forced to -1, regardless of which specific error.
//
// So the write-back below must happen ONLY on error, to turn that -1
// into a -1-valued vlong the C caller can see; on success it must leave
// the kernel's own write alone. That is exactly what principia's
// generator (9syscall/mkfile) emits -- "CMP $-1,R0; BNE 4(PC)" --
// i.e. skip the write-back unless R0 == -1.
//
// claude: this stub previously had the condition INVERTED (BEQ, writing
// back whenever the result was not -1), on the mistaken reading that
// the kernel returned the offset in the register and that principia's
// generator was the buggy one. It is the other way round. The effect
// was that every successful seek() overwrote the kernel's correct
// 8-byte result with whatever the return register happened to hold
// (under 5i, the stale syscall number), so seek() returned garbage --
// invisible until tests/c/hello_libc/io.c started CHECKING seek's
// return value rather than only its side effect. See
// notes_abi_plan9.txt.
TEXT seek(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$SEEK, R0
	SWI	$0
	MOVW	$-1, R2
	CMP	R2, R0
	BNE	seekdone
	MOVW	0(FP), R1
	MOVW	R0, 0(R1)
	MOVW	R0, 4(R1)
seekdone:
	RET
