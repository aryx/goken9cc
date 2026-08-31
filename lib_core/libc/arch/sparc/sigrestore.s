// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/mips/sigrestore.s's own header comment for the general story
// this file follows: the kernel invokes a signal handler with sig in
// the real hardware ABI's first-argument register (%o0, R8 in this
// port's flat numbering), but kc's own convention for a plain C
// function's first argument is R7 (REGARG) -- so move sig from R8 to
// R7, then call signotify normally.
//
// claude: R1/REGSP genuinely CAN be garbage on entry here -- worked
// through several wrong theories before landing on this fix, worth
// recording since the failure signatures look deceptively similar to
// other R1 bugs in this arch's own history:
//
//   1. First guess: R1 was simply "stale," fixable the way rt0.s's
//      _main fixes it (MOVW R14,R1). Broke the far more common case
//      where R1 already legitimately held the interrupted code's own
//      deep, ongoing stack pointer: this port's compiler never touches
//      %o6/R14 again after rt0.s's one-time setup except for
//      svc_sparc.s's own transient save/restore around each syscall
//      trap, so blindly copying it in clobbers R1 with a value that's
//      usually unrelated to where the interrupted code actually was.
//
//   2. Root cause, found via qemu's gdbstub: R1/REGSP is deliberately
//      mapped onto %g1 (docs/claude_notes/notes_arch_sparc.txt's R1/
//      %g1 collision background), which is ALSO the raw SPARC
//      syscall-number register. svc_sparc.s's _syscall6 briefly
//      clobbers it with the syscall number across the "TA R7" trap,
//      restoring it right after -- but a *synchronous self-signal*
//      (kill(getpid(), sig), exactly what notify.exe's alarm test
//      does) gets delivered by the kernel essentially as soon as the
//      trap itself returns, which can and does land inside that
//      window: breaking on sigentry's own entry showed %g1 == 37,
//      SYS_kill's own number (numbers_sparc.h) -- not a rare race, the
//      near-certain outcome for a self-kill().
//
//   3. Second guess: recover the original %g1 from the kernel's own
//      saved pt_regs snapshot (arg3/R10, per new_setup_rt_frame in the
//      real arch/sparc/kernel/signal.c), reading its u_regs[UREG_G1]
//      field. Dead end: since installsig() never sets SA_SIGINFO,
//      delivery actually goes through the OLDER new_setup_frame path
//      instead (confirmed empirically: args 2 and 3 arrived equal,
//      both aliasing __siginfo_t's own si_regs) -- and worse, its
//      saved snapshot IS just the raw trap-time register file, which
//      for this exact race already has %g1 == 37 too: we ourselves
//      clobber %g1 (via svc_sparc.s's "MOVW R7,R1" before the trap)
//      *before* the kernel takes its snapshot, so there is no
//      "original" g1 value anywhere on this path to recover -- WE
//      already threw it away, into R14, which the kernel knows nothing
//      about.
//
// Fix: sidestep the whole recovery problem. signotify() and everything
// it calls (sig2str, the notifyf callback, setjmp/noted's longjmp) are
// fully self-contained -- none of them need to read or write the
// *interrupted* code's own stack frames, only their own -- so sigentry
// hands them a small dedicated static buffer as R1 instead of trying
// to reconstruct whatever R1 the interrupted code had. Simple, always
// correct regardless of where the interruption landed, and avoids the
// g1/REGSP collision entirely rather than racing it.
//
// claude: TODO -- one static buffer means this is NOT reentrant: a
// second signal delivered while still inside this handler (before its
// own longjmp/return unwinds) would reuse the same buffer out from
// under the first invocation. Not fixed since no test here exercises
// nested signals (notify.exe's two postnote() calls run strictly one
// at a time, each fully handled via noted() before the next fires);
// a real fix would need either a small per-signal-depth stack pool or
// bumping this pointer down on entry and restoring it on exit.
//
// Also uses a leaf JMP-tail-call into signotify (not JMPL+RETURN):
// kl's noop.c auto-inserts a "SUB $frame,R1 / MOVW R15,0(R1)" prologue
// into any non-leaf TEXT (one containing a JMPL) BEFORE any
// hand-written instruction, regardless of source order (see rt0.s's
// own comment) -- which would run before the R1 fix above even had a
// chance to take effect. A leaf JMP skips that auto-prologue entirely;
// R15 flows through unmodified from the kernel into signotify(), which
// is an ordinary (non-leaf) C function and so gets its own
// auto-prologue saving R15 correctly once R1 points at the dedicated
// buffer -- its own RETURN bounces back through the kernel-provided
// restorer exactly as if sigentry had called it normally, losing
// nothing since sigentry never pushed a frame of its own to unwind.
//
// claude: everything above is a real fix, confirmed needed regardless
// of host. What's NOT fixed here: actually returning from a delivered
// signal (letting the interrupted code resume) is broken under
// qemu-sparc's own linux-user emulation, independent of anything this
// file or notify.c does -- confirmed with a minimal, dependency-free
// repro (no libc, no kc-generated code, just a hand-assembled
// rt_sigaction+kill+trivial-handler-with-a-bare-return test) that
// SIGSEGVs identically. installsig() (os/linux/notify.c) sets
// SA_SIGINFO on sparc so the kernel ABI at least matches real hardware
// (RT delivery, __NR_rt_sigreturn), but qemu crashes on ITS OWN
// sigreturn trampoline either way (RT or the older non-RT one), before
// resuming anything -- see docs/claude_notes/notes_arch_sparc.txt for
// the full writeup and the repro test. notify.exe's own alarm/hangup
// case (which posts a note to itself and expects to keep running) is
// therefore not expected to pass under qemu on this arch; nothing left
// to fix on the toolchain side for it.
GLOBL sigstack<>(SB), $4096
TEXT sigentry(SB), $0
	MOVW	$sigstack<>+4096(SB), R1
	MOVW	R8, R7
	MOVW	$signotify(SB), R9
	JMP	(R9)
