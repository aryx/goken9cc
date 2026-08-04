/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include <u.h>
#include <libc.h>

/* notify()/noted()/postnote() (include/os/plan9/note.h) for linux --
 * Tier 6 notification (docs/claude_notes/plan_syscalls.txt), the
 * Linux/darwin half docs/claude_notes/notes_libc_api_design.txt's
 * "Notes vs. signals" section designed and the Plan9 round
 * (notes_libc_selfhost.txt) left undone. port/atnotify.c (the real,
 * portable NFN-slot dispatch chain every actual caller in this tree
 * uses -- mk/Plan9.c, utilities/pipe/tee.c, utilities/text/misc/ed.c)
 * is UNCHANGED here: it only ever calls notify()/noted(), both
 * declared the same way on every GOOS, so this file just has to give
 * those two a real meaning on Linux -- everything above them is
 * already shared and already correct.
 *
 * Modeled directly on plan9port's own BOOT/lib9/notify.c (a real,
 * decades-proven implementation of exactly this problem, not
 * invented here): sigaction() over a curated signal set, and a
 * per-notification jmp_buf standing in for noted()'s "tell the kernel
 * what to do next" contract, which this project can build for real
 * now that Tier 7 (setjmp/longjmp) exists. Deliberately the SIMPLE
 * (non-SA_SIGINFO) handler shape -- lib9's own signotify(int sig)
 * takes no siginfo_t/ucontext_t either, since nothing downstream
 * (this dispatcher, or any real atnotify() handler in this tree) uses
 * anything beyond the signal number.
 *
 * The curated signal set is deliberately narrow: SIGHUP/SIGINT/
 * SIGQUIT/SIGALRM/SIGTERM/SIGPIPE, six signals whose real POSIX
 * default disposition is uniformly "terminate the process" -- the
 * same disposition noted(NDFLT) below gives them (a plain exit(1)),
 * so this file does not need to reimplement "restore SIG_DFL and
 * re-raise" for real. Fault signals (SIGSEGV/SIGBUS/SIGILL/SIGFPE)
 * and job-control ones (SIGCHLD/SIGTSTP/...) are deliberately left
 * alone, matching lib9's own exclusions (its header comment: "We do
 * not handle SIGABRT or SIGSEGV..."). SIGUSR1/SIGUSR2 are also left
 * out of this first cut, NOT because they are hard, but because their
 * signal NUMBERS differ on mips (16/17) from every other arch here
 * (10/12) -- HUP/INT/QUIT/ALRM/TERM/PIPE are numerically identical on
 * every arch in this tree, mips included, confirmed against real
 * kernel source (arch/mips/include/uapi/asm/signal.h vs the generic
 * asm-generic one), so this table needs no per-arch numbers at all.
 * An unrecognized postnote() string simply fails (-1) rather than
 * guessing a fallback signal -- more honest than picking one
 * arbitrarily, and matches the real postnote()'s own `default: return
 * -1;` for an unrecognized group argument (os/plan9/postnote.c).
 */

/* Ksigaction (the real per-arch kernel struct) and SA_RESTORER_VAL/
 * __NR_rt_sigreturn live in syscall/os/linux/numbers_$cputype.h, same
 * as every raw syscall number in this tree -- but this file is SHARED
 * across every arch (unlike os/linux/stat_$cputype.c's own per-arch
 * split), so it cannot just #include "numbers_amd64.h" by name. This
 * compiler's preprocessor has no #if expression support at all (only
 * plain #ifdef/#ifndef/#else/#endif -- see CLAUDE.md), so a single
 * `#if defined(amd64) || ...`-style guard isn't available either: one
 * #ifdef per arch, selecting which real header to pull in, is the
 * only mechanism this build actually supports. Each -D$cputype (see
 * this mkfile's own CFLAGS) makes exactly one of these true.
 */
#ifdef amd64
#include "syscall/os/linux/numbers_amd64.h"
#endif
/* claude: 386 checked via `cputype386` (mkfiles/386/mkfile's own -D),
 * not the usual bare -D$cputype -- a purely-numeric #ifdef operand
 * like plain `386` fails outright ("syntax in #if(n)def"), the same
 * pitfall port/getcallerpc.c's own #ifdef cputype386 already
 * documents (and this file hit for real before this comment existed:
 * the naive `#ifdef 386` compiled clean for every OTHER arch but
 * broke exactly this one, its errors pointing at numbers_386.h lines
 * that had no business being pulled into an amd64 build at all). */
#ifdef cputype386
#include "syscall/os/linux/numbers_386.h"
#endif
#ifdef arm
#include "syscall/os/linux/numbers_arm.h"
#endif
#ifdef arm64
#include "syscall/os/linux/numbers_arm64.h"
#endif
#ifdef mips
#include "syscall/os/linux/numbers_mips.h"
#endif
#ifdef riscv
#include "syscall/os/linux/numbers_riscv.h"
#endif
#ifdef riscv64
#include "syscall/os/linux/numbers_riscv64.h"
#endif

extern int _sysrtsigaction(int sig, void *act, void *oact, long sigsetsize);
extern int _syskill(int pid, int sig);
#ifdef amd64
extern void sigrestorer(void);
#endif

/* claude: sigentry (arch/$cputype/sigrestore.s), NOT signotify below,
 * is what actually gets installed as sa.handler, on every arch --
 * even the ones that turn out not to strictly need it (see below),
 * so this file itself stays arch-uniform. Found the hard way, not
 * designed in from the start: a first version installed signotify (a
 * plain C function taking one int) directly, and on amd64 the handler
 * always observed sig=1 regardless of which signal was actually sent.
 * Root cause: the KERNEL invokes a signal handler using the real
 * hardware/OS ABI (sig in EDI/RDI on amd64, a fixed, non-negotiable
 * convention every signal-capable program on that kernel must match)
 * -- but this compiler's OWN C calling convention is stack-based even
 * on amd64 (confirmed against arch/amd64/rt0.s's own comment: "amd64
 * has no register-passed argument at all... every argument, including
 * a callee's first, is written to the stack by the *caller*"), so
 * signotify's own prologue was reading its `sig` parameter from a
 * stack slot the kernel never wrote anything meaningful into.
 * sigentry bridges this exactly the way rt0.s already bridges the
 * kernel's raw process-entry convention into a proper call to main()
 * -- read sig from the real ABI's register/stack slot, write it to
 * wherever THIS compiler's own CALL convention expects a first
 * argument, then call signotify normally.
 *
 * Tested per arch, not assumed uniform from one arch's fix (an
 * isolated single-signal probe, built with goken's own compiler and
 * run under that arch's own qemu-user emulator, iterated until each
 * one actually worked rather than stopping at "compiles/asssembles
 * clean"): amd64/386/mips/riscv/riscv64 all needed a real bridging
 * sigentry, though for two DIFFERENT reasons found separately. amd64/
 * mips/riscv/riscv64 have the argument-register mismatch described
 * above (sig read as garbage -- concretely, the previous syscall's own
 * number, evidence the compiler's first-parameter convention for a
 * plain C function is stack- or scratch-register-based on these, not
 * the real kernel ABI's register). 386's own kernel ABI happens to
 * pass sig on the stack at the SAME offset this compiler's own
 * convention already expects (cdecl has no register-passed args at
 * all), so argument reading alone works even with signotify installed
 * directly -- but a bare `JMP signotify(SB)` as sa.handler segfaults
 * on entry regardless (confirmed in isolation: unlike a same-file `JMP
 * label` such as arch/386/rt0.s's own `JMP loop`, a plain PC-relative
 * local branch, jumping to another compilation unit's external symbol
 * this way does not work as a raw kernel-invoked entry point), so 386
 * needs the same CALL-based staging dance as amd64 for an unrelated
 * reason. arm/arm64 are the only arches where a plain, direct
 * installation of signotify actually works end to end with zero
 * bridging -- they still get a (trivial, single-instruction tail-
 * branch) sigentry, purely so installsig() below never needs a
 * per-arch branch of its own.
 *
 * SA_RESTORER is a separate axis, independently arch-specific and NOT
 * correlated with whether sigentry does real bridging work: only
 * amd64 actually needs it (confirmed: omitting it there leaves the
 * handler running but the process never correctly resumes). mips has
 * no such kernel field at all (numbers_mips.h's own comment). Every
 * other arch here (386, arm, arm64, riscv, riscv64) was tested WITHOUT
 * it and returned cleanly -- for 386 specifically, adding SA_RESTORER
 * back in (with the argument-bridging fix already in place) was
 * tested too and reliably SEGFAULTS on resume, a real, separately
 * confirmed regression, not just "didn't bother trying it". Root
 * cause of that specific crash not fully chased down (suspected i386
 * rt_sigframe layout mismatch); the working/broken split itself is
 * reproduced multiple times, not a fluke.
 */
extern void sigentry(void);

#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT	3
#define SIGALRM	14
#define SIGTERM	15
#define SIGPIPE	13

static struct {
	int	sig;
	char	*str;
} sigtab[] = {
	SIGHUP,		"hangup",
	SIGINT,		"interrupt",
	SIGQUIT,	"quit",
	SIGALRM,	"alarm",
	SIGTERM,	"kill",
	SIGPIPE,	"sys: write on closed pipe",
};

static char*
sig2str(int sig, char *tmp)
{
	int i;

	for(i = 0; i < nelem(sigtab); i++)
		if(sigtab[i].sig == sig)
			return sigtab[i].str;
	sprint(tmp, "sys: signal %d", sig);
	return tmp;
}

static int
str2sig(char *s)
{
	int i;

	for(i = 0; i < nelem(sigtab); i++)
		if(strcmp(s, sigtab[i].str) == 0)
			return sigtab[i].sig;
	return 0;
}

static void (*notifyf)(void*, char*);
static jmp_buf notejmp;

/* claude: NOT static -- arch/$cputype/sigentry.s (a different
 * compilation unit) calls this by name directly (`CALL signotify(SB)`)
 * after bridging the kernel's raw signal-delivery ABI into a proper
 * call, so it needs external linkage. Not part of any public header,
 * just not file-local either. */
void
signotify(int sig)
{
	char tmp[32];
	char *s;

	switch(setjmp(notejmp)){
	case 0:
		s = sig2str(sig, tmp);
		if(notifyf)
			(*notifyf)(nil, s);
		/* fall through: handler returned without calling noted()
		 * at all -- same as NDFLT, matching lib9's own signotify() */
	case 1:	/* noted(NDFLT) */
		exit(1);
	case 2:	/* noted(NCONT) */
		return;
	}
}

int
noted(int v)
{
	longjmp(notejmp, v == NCONT ? 2 : 1);
	return 0;	/* unreachable */
}

static void
installsig(int sig, int on)
{
	Ksigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.handler = on ? (void(*)(int))sigentry : nil;
#ifdef amd64
	if(on){
		sa.flags = SA_RESTORER_VAL;
		sa.restorer = sigrestorer;
	}
#endif
	_sysrtsigaction(sig, &sa, nil, sizeof sa.mask);
}

int
notify(void (*f)(void*, char*))
{
	int i;

	notifyf = f;
	for(i = 0; i < nelem(sigtab); i++)
		installsig(sigtab[i].sig, f != nil);
	return 0;
}

int
postnote(int group, int pid, char *note)
{
	int sig;

	sig = str2sig(note);
	if(sig == 0)
		return -1;
	switch(group){
	case PNPROC:
		return _syskill(pid, sig) < 0 ? -1 : 0;
	case PNGROUP:
		return _syskill(-pid, sig) < 0 ? -1 : 0;
	}
	return -1;
}
