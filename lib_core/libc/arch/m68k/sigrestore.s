// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/mips/sigrestore.s's own header comment for the general story
// this file follows.
//
// claude: the ORIGINAL version of this file (BSR signotify(SB) / RTS)
// was wrong, despite its own comment's reasoning being half right: it
// IS true that m68k needs no register bridge for `sig` the way
// sparc/mips do (2c's own calling convention is stack-based, and the
// real kernel's signal-frame layout -- struct sigframe {pretcode;
// sig; code; ...}, confirmed against arch/m68k/kernel/signal.c -- is
// ALSO stack-based and lines up with kc's own $FP convention: A7 at
// handler entry points at `pretcode` exactly the way it would point
// at a real return address pushed by a caller, so a PLAIN function
// with one "int sig" parameter would already read frame->sig
// correctly via sig+0(FP)). The bug: this file used BSR, a REAL call,
// which pushes SIGENTRY's OWN return address first -- shifting
// EVERYTHING down by 4 bytes, so signotify(int sig) ended up reading
// sig+0(FP) as *frame->pretcode* instead (the raw trampoline
// instruction bytes the kernel wrote there, misread as an int).
// Symptom: notify.exe's alarm test never crashed, just silently
// exited 1 every time -- sig2str() converting garbage never matched
// any registered handler's note string, so signotify's own switch
// fell through to case 1 (NDFLT) unconditionally, confirmed via
// qemu-strace showing a clean "exit(1)" right after the SIGALRM
// delivery line, no crash to chase.
//
// Fix: a plain tail JMP instead of BSR+RTS -- no linkage pushed at
// all, so signotify sees the EXACT SAME stack sigentry was handed
// (sig+0(FP) resolves correctly), and its own auto-generated epilogue
// RTS pops *frame->pretcode* directly -- the kernel's own retcode
// trampoline -- exactly as if the kernel had jumped to signotify
// itself in the first place.
TEXT sigentry(SB), $0
	JMP	signotify(SB)
