// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "defs.h"
#include "signals.h"
#include "os.h"

extern void sigtramp(void);

typedef struct sigaction {
	union {
		void    (*__sa_handler)(int32);
		void    (*__sa_sigaction)(int32, Siginfo*, void *);
	} __sigaction_u;		/* signal handler */
	int32	sa_flags;		/* see signal options below */
	int64	sa_mask;		/* signal mask to apply */
} Sigaction;

void
dumpregs(Mcontext *r)
{
	printf("rax     %X\n", r->mc_rax);
	printf("rbx     %X\n", r->mc_rbx);
	printf("rcx     %X\n", r->mc_rcx);
	printf("rdx     %X\n", r->mc_rdx);
	printf("rdi     %X\n", r->mc_rdi);
	printf("rsi     %X\n", r->mc_rsi);
	printf("rbp     %X\n", r->mc_rbp);
	printf("rsp     %X\n", r->mc_rsp);
	printf("r8      %X\n", r->mc_r8 );
	printf("r9      %X\n", r->mc_r9 );
	printf("r10     %X\n", r->mc_r10);
	printf("r11     %X\n", r->mc_r11);
	printf("r12     %X\n", r->mc_r12);
	printf("r13     %X\n", r->mc_r13);
	printf("r14     %X\n", r->mc_r14);
	printf("r15     %X\n", r->mc_r15);
	printf("rip     %X\n", r->mc_rip);
	printf("rflags  %X\n", r->mc_flags);
	printf("cs      %X\n", r->mc_cs);
	printf("fs      %X\n", r->mc_fs);
	printf("gs      %X\n", r->mc_gs);
}

String
signame(int32 sig)
{
	if(sig < 0 || sig >= NSIG)
		return emptystring;
	return gostringnocopy((byte*)sigtab[sig].name);
}

void
sighandler(int32 sig, Siginfo* info, void* context)
{
	Ucontext *uc;
	Mcontext *r;
	G *gp;
	uintptr *sp;

	uc = context;
	r = &uc->uc_mcontext;

	if((gp = m->curg) != nil && (sigtab[sig].flags & SigPanic)) {
		// Make it look like a call to the signal func.
		// Have to pass arguments out of band since
		// augmenting the stack frame would break
		// the unwinding code.
		gp->sig = sig;
		gp->sigcode0 = info->si_code;
		gp->sigcode1 = (uintptr)info->si_addr;

		// Only push sigpanic if r->mc_rip != 0.
		// If r->mc_rip == 0, probably panicked because of a
		// call to a nil func.  Not pushing that onto sp will
		// make the trace look like a call to sigpanic instead.
		// (Otherwise the trace will end at sigpanic and we
		// won't get to see who faulted.)
		if(r->mc_rip != 0) {
			sp = (uintptr*)r->mc_rsp;
			*--sp = r->mc_rip;
			r->mc_rsp = (uintptr)sp;
		}
		r->mc_rip = (uintptr)sigpanic;
		return;
	}

	if(sigtab[sig].flags & SigQueue) {
		if(sigsend(sig) || (sigtab[sig].flags & SigIgnore))
			return;
		exit(2);	// SIGINT, SIGTERM, etc
	}

	if(panicking)	// traceback already printed
		exit(2);
	panicking = 1;

	if(sig < 0 || sig >= NSIG)
		printf("Signal %d\n", sig);
	else
		printf("%s\n", sigtab[sig].name);

	printf("PC=%X\n", r->mc_rip);
	printf("\n");

	if(gotraceback()){
		traceback((void*)r->mc_rip, (void*)r->mc_rsp, 0, g);
		tracebackothers(g);
		dumpregs(r);
	}

	breakpoint();
	exit(2);
}

void
sigignore(void)
{
}

void
signalstack(byte *p, int32 n)
{
	Sigaltstack st;

	st.ss_sp = (int8*)p;
	st.ss_size = n;
	st.ss_flags = 0;
	sigaltstack(&st, nil);
}

void
initsig(int32 queue)
{
	static Sigaction sa;

	siginit();

	int32 i;
	sa.sa_flags |= SA_ONSTACK | SA_SIGINFO;
	sa.sa_mask = ~0x0ull;
	
	for(i = 0; i < NSIG; i++) {
		if(sigtab[i].flags) {
			if((sigtab[i].flags & SigQueue) != queue)
				continue;
			if(sigtab[i].flags & (SigCatch | SigQueue))
				sa.__sigaction_u.__sa_sigaction = (void*) sigtramp;
			else
				sa.__sigaction_u.__sa_sigaction = (void*) sigignore;

			if(sigtab[i].flags & SigRestart)
				sa.sa_flags |= SA_RESTART;
			else
				sa.sa_flags &= ~SA_RESTART;

			sigaction(i, &sa, nil);
		}
	}
}
