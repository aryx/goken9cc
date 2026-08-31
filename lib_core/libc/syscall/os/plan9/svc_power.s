// Raw Plan9 (GOOS=plan9) syscall stubs for power. See svc_arm.s's own
// header comment for the general "no shared trampoline needed" story
// -- everything there applies here too, just with this arch's own
// registers/trap: qc passes this function's own FIRST argument (num)
// in R3 (REGARG==REGRET==3, confirmed via 'qc -S' on a two-arg
// function -- the same register linux/power/svc_power.s's own header
// comment already established), every OTHER argument packs at FP+4,
// FP+8, ... (power's pointers are 4 bytes, no padding -- same fact
// linux/power/svc_power.s cites against tests/c/vlong's own write()
// stub), and SYSCALL (real PowerPC `sc`) is the trap instruction.
// Unlike linux/power's own _syscall6, which has real error-flag
// (CR0.SO) conversion work to do, Plan9's kernel returns its result
// directly in R3 -- the same register the syscall number went in, and
// the same register this compiler's own calling convention already
// returns through -- so no post-trap conversion is needed here either.
//
// Directly transcribed from the real Plan9 4th edition source
// (~/xxx/plan9/SRC/libc/9syscall/mkfile's per-syscall generator, the
// `case power` branch -- not principia's), except RETURN's argument
// count (this mkfile's own `, 1, $0` vs. goken's plain `$0` TEXT
// convention, matching every other svc_$cputype.s here). seek()'s
// scratch register (R8) and CMP+BNE write-back guard are transcribed
// the same way, and independently match what 'qc -S' emits for a real
// `if (a == -1)` (`CMP R3,$-1 / BNE ,N(PC)`, no temp register needed
// to hold the -1 comparand, unlike arm's MOVW $-1,R2 dance).
#include "sys.h"

// exits(char *msg) -- see svc_arm.s's identical comment.
TEXT exits(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$EXITS, R3
	SYSCALL
	RETURN

TEXT open(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$OPEN, R3
	SYSCALL
	RETURN

TEXT close(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$CLOSE, R3
	SYSCALL
	RETURN

// create/remove/chdir -- see svc_arm.s's identical comment on why these
// need no os/plan9/ translation glue, unlike every other GOOS.
TEXT create(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$CREATE, R3
	SYSCALL
	RETURN

TEXT remove(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$REMOVE, R3
	SYSCALL
	RETURN

TEXT chdir(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$CHDIR, R3
	SYSCALL
	RETURN

// dup(oldfd, newfd) -- see svc_arm.s's identical comment.
TEXT dup(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$DUP, R3
	SYSCALL
	RETURN

// brk(void*) -- see svc_arm.s's identical comment (port/sbrk.c's
// primitive).
TEXT brk(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$BRK, R3
	SYSCALL
	RETURN

// fd2path(fd, buf, nbuf) -- see svc_arm.s's identical comment.
TEXT fd2path(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$FD2PATH, R3
	SYSCALL
	RETURN

// sleep(ms) -- see svc_arm.s's identical comment.
TEXT sleep(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$SLEEP, R3
	SYSCALL
	RETURN

// alarm(ms) -- see svc_arm.s's identical comment. NOT yet implemented
// by qi (machines/qi/syscall.c's sysalarm() is a "No system call"
// stub), so a -H2 binary calling this dies under the emulator even
// though it would work on real hardware. See todo.org.
TEXT alarm(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$ALARM, R3
	SYSCALL
	RETURN

TEXT pread(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$PREAD, R3
	SYSCALL
	RETURN

TEXT pwrite(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$PWRITE, R3
	SYSCALL
	RETURN

// fstat/fwstat -- see svc_arm.s's identical comment.
TEXT fstat(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$FSTAT, R3
	SYSCALL
	RETURN

TEXT fwstat(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$FWSTAT, R3
	SYSCALL
	RETURN

// seek's raw syscall return doesn't fit the usual "result in R3"
// shape -- see svc_arm.s's identical, much longer comment for the
// full story (the kernel writes the 8-byte result through a hidden
// pointer passed as this function's own first argument, and the
// write-back below must happen ONLY on error, to turn a raw -1 into a
// -1-valued vlong the C caller can see).
TEXT seek(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$SEEK, R3
	SYSCALL
	CMP	R3, $-1
	BNE	seekdone
	MOVW	0(FP), R8
	MOVW	R3, 0(R8)
	MOVW	R3, 4(R8)
seekdone:
	RETURN

// claude: Tier 4 process control -- see svc_arm.s's identical comment.
TEXT rfork(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$RFORK, R3
	SYSCALL
	RETURN

TEXT exec(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$EXEC, R3
	SYSCALL
	RETURN

TEXT await(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$AWAIT, R3
	SYSCALL
	RETURN

TEXT pipe(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$PIPE, R3
	SYSCALL
	RETURN

// claude: Tier 6 notification -- see svc_arm.s's identical comment.
TEXT notify(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$NOTIFY, R3
	SYSCALL
	RETURN

TEXT noted(SB), $0
	MOVW	R3, 0(FP)
	MOVW	$NOTED, R3
	SYSCALL
	RETURN
