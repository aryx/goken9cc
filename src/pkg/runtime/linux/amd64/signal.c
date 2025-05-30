// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "defs.h"
#include "signals.h"
#include "os.h"

void
dumpregs(Sigcontext *r)
{
	printf("rax     %X\n", r->rax);
	printf("rbx     %X\n", r->rbx);
	printf("rcx     %X\n", r->rcx);
	printf("rdx     %X\n", r->rdx);
	printf("rdi     %X\n", r->rdi);
	printf("rsi     %X\n", r->rsi);
	printf("rbp     %X\n", r->rbp);
	printf("rsp     %X\n", r->rsp);
	printf("r8      %X\n", r->r8 );
	printf("r9      %X\n", r->r9 );
	printf("r10     %X\n", r->r10);
	printf("r11     %X\n", r->r11);
	printf("r12     %X\n", r->r12);
	printf("r13     %X\n", r->r13);
	printf("r14     %X\n", r->r14);
	printf("r15     %X\n", r->r15);
	printf("rip     %X\n", r->rip);
	printf("rflags  %X\n", r->eflags);
	printf("cs      %X\n", (uint64)r->cs);
	printf("fs      %X\n", (uint64)r->fs);
	printf("gs      %X\n", (uint64)r->gs);
}

/*
 * This assembler routine takes the args from registers, puts them on the stack,
 * and calls sighandler().
 */
extern void sigtramp(void);
extern void sigignore(void);	// just returns
extern void sigreturn(void);	// calls sigreturn

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
	Mcontext *mc;
	Sigcontext *r;
	uintptr *sp;
	G *gp;

	uc = context;
	mc = &uc->uc_mcontext;
	r = (Sigcontext*)mc;	// same layout, more conveient names

	if((gp = m->curg) != nil && (sigtab[sig].flags & SigPanic)) {
		// Make it look like a call to the signal func.
		// Have to pass arguments out of band since
		// augmenting the stack frame would break
		// the unwinding code.
		gp->sig = sig;
		gp->sigcode0 = info->si_code;
		gp->sigcode1 = ((uintptr*)info)[2];

		// Only push sigpanic if r->rip != 0.
		// If r->rip == 0, probably panicked because of a
		// call to a nil func.  Not pushing that onto sp will
		// make the trace look like a call to sigpanic instead.
		// (Otherwise the trace will end at sigpanic and we
		// won't get to see who faulted.)
		if(r->rip != 0) {
			sp = (uintptr*)r->rsp;
			*--sp = r->rip;
			r->rsp = (uintptr)sp;
		}
		r->rip = (uintptr)sigpanic;
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

	printf("PC=%X\n", r->rip);
	printf("\n");

	if(gotraceback()){
		traceback((void*)r->rip, (void*)r->rsp, 0, g);
		tracebackothers(g);
		dumpregs(r);
	}

	breakpoint();
	exit(2);
}

void
signalstack(byte *p, int32 n)
{
	Sigaltstack st;

	st.ss_sp = p;
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
	sa.sa_flags = SA_ONSTACK | SA_SIGINFO | SA_RESTORER;
	sa.sa_mask = 0xFFFFFFFFFFFFFFFFULL;
	sa.sa_restorer = (void*)sigreturn;
	for(i = 0; i<NSIG; i++) {
		if(sigtab[i].flags) {
			if((sigtab[i].flags & SigQueue) != queue)
				continue;
			if(sigtab[i].flags & (SigCatch | SigQueue))
				sa.sa_handler = (void*)sigtramp;
			else
				sa.sa_handler = (void*)sigignore;
			if(sigtab[i].flags & SigRestart)
				sa.sa_flags |= SA_RESTART;
			else
				sa.sa_flags &= ~SA_RESTART;
			rt_sigaction(i, &sa, nil, 8);
		}
	}
}
