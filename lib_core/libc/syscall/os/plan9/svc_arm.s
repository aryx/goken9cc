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

// dup(oldfd, newfd) -- the real kernel call, already exactly the shape
// include/os/file.h declares, newfd == -1 ("lowest free fd") included.
// On every other GOOS this needs port/dup.c to pick between POSIX's
// dup() and dup2(); here the one syscall covers both cases itself (see
// principia's kernel/files/sysfile.c sysdup, whose `if(fd != -1)`
// branch is where that split comes from in the first place).
//
// access() is the mirror image and is NOT here: it is a real syscall on
// every other GOOS, but on Plan9 it is a libc function built out of
// open()/stat() -- see os/plan9/access.c.
TEXT dup(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$DUP, R0
	SWI	$0
	RET

// claude: brk(void*) -- the memory primitive port/sbrk.c is built on.
// Plan9 is the one GOOS where it needs no glue at all: the kernel call
// is already both named and shaped like include/os/mem.h's brk()
// (absolute new-break address in, 0 or -1 out), so port/sbrk.c links
// straight against this. Compare os/linux/brk.c, which exists purely to
// convert Linux's return-the-new-break-and-no-errno convention into
// this one, and os/darwin/sbrk.c, which has no brk to call at all.
//
// Note it is brk here, not principia's brk_: that name only exists in
// Plan9's own libc because 9sys/sbrk.c wants the plain `brk` symbol for
// a wrapper it no longer needs once brk is already this shape.
//
// 5i implements BRK (machines/5i/syscall.c's sysbrk), so this runs under
// the emulator with no machines/ work.
TEXT brk(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$BRK, R0
	SWI	$0
	RET

// claude: fd2path(fd, buf, nbuf) -- "what path is this descriptor?".
// No Unix has an equivalent syscall, which is exactly why os/plan9/
// getwd.c can be three lines while the other GOOSes each need their own
// mechanism: linux has a real getcwd(2), and darwin (which has none
// either) reaches for fcntl(F_GETPATH), the closest thing to this call
// outside Plan9. 5i implements FD2PATH already (machines/5i/
// syscall_posix.c's sysfd2path), so getwd runs under the emulator.
TEXT fd2path(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$FD2PATH, R0
	SWI	$0
	RET

// claude: sleep(ms) -- milliseconds, matching include/os/time.h's
// sleep(long) exactly, so no glue at all on this GOOS (contrast
// os/linux/time.c, which has to build a timespec, and os/darwin/time.c,
// which has no nanosleep syscall to call in the first place). sleep(0)
// means yield. 5i implements SLEEP (syssleep).
TEXT sleep(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$SLEEP, R0
	SWI	$0
	RET

// claude: alarm(ms) -- also milliseconds, and also a direct syscall on
// this GOOS, so again no glue file: contrast os/linux/alarm.c and
// os/darwin/alarm.c, which both have to build an itimerval and call
// setitimer because neither kernel has a millisecond one-shot timer of
// its own. This is the ORIGINAL of the API the other three implement;
// the note it posts on expiry ("alarm") is the kernel's, not a name
// this libc invented. Returns the milliseconds left on the previous
// alarm, 0 if none was pending -- the return value arrives in R0
// exactly like open()'s fd, so no post-processing here either.
// NOT yet implemented by 5i or vi: machines/5i/syscall.c's sysalarm()
// and machines/vi/syscall.c's are both "No system call" stubs, so a
// -H2 binary calling this dies under the emulators even though it
// would work on real hardware. See todo.org.
TEXT alarm(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$ALARM, R0
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

// claude: fstat(fd, buf, nbuf) / fwstat(fd, buf, nbuf) -- the raw
// machine-independent-buffer syscalls Tier 3's dirfstat()/dirfwstat()
// (os/plan9/stat.c) unpack/pack via convM2D/convD2M (os/plan9/
// convM2D.c, convD2M.c). Same plain "result in R0" shape as open/
// close/create/remove/chdir/dup/brk/pread/pwrite above -- no vlong
// write-back dance like seek needs, since the result here is just an
// int byte count, not a wider value the kernel writes through a
// hidden pointer.
TEXT fstat(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$FSTAT, R0
	SWI	$0
	RET

TEXT fwstat(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$FWSTAT, R0
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

// claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
// rfork(int)/exec(char*,char*[])/await(char*,int)/pipe(int*) are
// already exactly the shape include/os/proc.h and include/os/ipc.h
// declare -- unlike every Linux/Darwin arch, which needs a port/*.c or
// os/$GOOS/*.c bridge to bond a raw syscall onto Plan9's own API shape,
// here the raw syscall ALREADY IS that public API, the same story as
// open/close/create/remove/chdir/dup/brk above. So these are four more
// one-line stubs, no different in kind from those -- fork()/wait() are
// the two functions actually built on top of these (os/plan9/fork.c,
// os/plan9/wait.c), not raw syscalls themselves, so they don't belong
// in this file.
TEXT rfork(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$RFORK, R0
	SWI	$0
	RET

TEXT exec(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$EXEC, R0
	SWI	$0
	RET

TEXT await(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$AWAIT, R0
	SWI	$0
	RET

TEXT pipe(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$PIPE, R0
	SWI	$0
	RET

// claude: Tier 6 notification (docs/claude_notes/plan_syscalls.txt).
// notify(void(*)(void*,char*))/noted(int) are the two real syscalls
// this tier needs (sys.h: NOTIFY=25, NOTED=26) -- same one-line-stub
// shape as rfork/exec/await/pipe above, since both already match
// include/os/plan9/note.h's own signature with nothing to translate.
// postnote() is NOT a syscall (os/plan9/postnote.c: a plain write() to
// /proc/PID/note), so it doesn't belong here.
TEXT notify(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$NOTIFY, R0
	SWI	$0
	RET

TEXT noted(SB), $0
	MOVW	R0, 0(FP)
	MOVW	$NOTED, R0
	SWI	$0
	RET
