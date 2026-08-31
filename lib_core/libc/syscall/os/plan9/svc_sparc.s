// Raw Plan9 (GOOS=plan9) syscall stubs for sparc. See svc_arm.s's own
// header comment for the general "no shared trampoline needed" story
// -- everything there applies here too, just with this arch's own
// registers/trap: kc passes this function's own FIRST argument (num)
// in R7 (REGARG==REGRET==7, confirmed via 'kc -S' on a two-arg
// function -- the same register linux/sparc/svc_sparc.s's own header
// comment already established), every OTHER argument packs at FP+4,
// FP+8, ... , and TA R0 (SPARC's "trap always", trapping with vector 0
// since %g0/R0 is hardwired zero -- this is Plan9's own convention,
// NOT Linux's "TA 0x10", see linux/sparc/svc_sparc.s) is the trap
// instruction. There is no bare SYSCALL mnemonic for this arch (see
// assemblers/ka's own mnemonic table).
//
// Unlike linux/sparc's own _syscall6 (syscall/os/linux/svc_sparc.s),
// this needs NONE of that file's R1(REGSP)/%g1 save-restore dance:
// that collision is a *real hardware* fact (kc's own logical R1 IS the
// real %g1 the Linux kernel's `ta 0x10` trap reads), but machines/ki
// is a software emulator whose ta() handler (machines/ki/syscall.c)
// only ever reads REGSP (R1) and REGRET (R7) as plain array slots in
// its own emulated Registers struct -- there is no physical %g1 for
// the trap itself to disturb, so R1 (this function's own logical
// stack pointer, used for every FP-relative argument read below) is
// never at risk.
//
// Directly transcribed from the real Plan9 4th edition source
// (~/xxx/plan9/SRC/libc/9syscall/mkfile's per-syscall generator, the
// `case sparc sparc64` branch -- not principia's, and not derived by
// analogy with arm/mips/an emulator-only empirical test the way an
// earlier draft of this file did, which got the trap operand wrong
// (used Linux's "$0x10,R11" instead of Plan9's own "TA R0") since
// machines/ki's own ta() dispatch happens to accept any trap operand
// and couldn't have caught the mismatch), except RETURN's argument
// count (this mkfile's own `, 1, $0` vs. goken's plain `$0` TEXT
// convention, matching every other svc_$cputype.s here). seek()'s
// scratch register (R8) and CMP+BNE write-back guard are transcribed
// the same way, and independently match what 'kc -S' emits for a real
// `if (a == -1)`.
#include "sys.h"

// exits(char *msg) -- see svc_arm.s's identical comment.
TEXT exits(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$EXITS, R7
	TA	R0
	RETURN

TEXT open(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$OPEN, R7
	TA	R0
	RETURN

TEXT close(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$CLOSE, R7
	TA	R0
	RETURN

// create/remove/chdir -- see svc_arm.s's identical comment on why these
// need no os/plan9/ translation glue, unlike every other GOOS.
TEXT create(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$CREATE, R7
	TA	R0
	RETURN

TEXT remove(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$REMOVE, R7
	TA	R0
	RETURN

TEXT chdir(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$CHDIR, R7
	TA	R0
	RETURN

// dup(oldfd, newfd) -- see svc_arm.s's identical comment.
TEXT dup(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$DUP, R7
	TA	R0
	RETURN

// brk(void*) -- see svc_arm.s's identical comment (port/sbrk.c's
// primitive).
TEXT brk(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$BRK, R7
	TA	R0
	RETURN

// fd2path(fd, buf, nbuf) -- see svc_arm.s's identical comment.
TEXT fd2path(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$FD2PATH, R7
	TA	R0
	RETURN

// sleep(ms) -- see svc_arm.s's identical comment.
TEXT sleep(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$SLEEP, R7
	TA	R0
	RETURN

// alarm(ms) -- see svc_arm.s's identical comment. NOT yet implemented
// by ki (machines/ki/syscall.c's sysalarm() is a "No system call"
// stub), so a -H2 binary calling this dies under the emulator even
// though it would work on real hardware. See todo.org.
TEXT alarm(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$ALARM, R7
	TA	R0
	RETURN

TEXT pread(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$PREAD, R7
	TA	R0
	RETURN

TEXT pwrite(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$PWRITE, R7
	TA	R0
	RETURN

// fstat/fwstat -- see svc_arm.s's identical comment.
TEXT fstat(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$FSTAT, R7
	TA	R0
	RETURN

TEXT fwstat(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$FWSTAT, R7
	TA	R0
	RETURN

// seek's raw syscall return doesn't fit the usual "result in R7"
// shape -- see svc_arm.s's identical, much longer comment for the
// full story (the kernel writes the 8-byte result through a hidden
// pointer passed as this function's own first argument, and the
// write-back below must happen ONLY on error, to turn a raw -1 into a
// -1-valued vlong the C caller can see).
TEXT seek(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$SEEK, R7
	TA	R0
	CMP	R7, $-1
	BNE	seekdone
	MOVW	0(FP), R8
	MOVW	R7, 0(R8)
	MOVW	R7, 4(R8)
seekdone:
	RETURN

// claude: Tier 4 process control -- see svc_arm.s's identical comment.
TEXT rfork(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$RFORK, R7
	TA	R0
	RETURN

TEXT exec(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$EXEC, R7
	TA	R0
	RETURN

TEXT await(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$AWAIT, R7
	TA	R0
	RETURN

TEXT pipe(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$PIPE, R7
	TA	R0
	RETURN

// claude: Tier 6 notification -- see svc_arm.s's identical comment.
TEXT notify(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$NOTIFY, R7
	TA	R0
	RETURN

TEXT noted(SB), $0
	MOVW	R7, 0(FP)
	MOVW	$NOTED, R7
	TA	R0
	RETURN
