// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

//
// System calls and other sys.stuff for AMD64, Linux
//

#include "amd64/asm.h"

TEXT	exit(SB),7,$0-8
	MOVL	8(SP), DI
	MOVL	$231, AX	// exitgroup - force all os threads to exit
	SYSCALL
	RET

TEXT	exit1(SB),7,$0-8
	MOVL	8(SP), DI
	MOVL	$60, AX	// exit - exit the current os thread
	SYSCALL
	RET

TEXT	open(SB),7,$0-16
	MOVQ	8(SP), DI
	MOVL	16(SP), SI
	MOVL	20(SP), DX
	MOVL	$2, AX			// syscall entry
	SYSCALL
	RET

TEXT	write(SB),7,$0-24
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	$1, AX			// syscall entry
	SYSCALL
	RET

TEXT	gettime(SB), 7, $32
	LEAQ	8(SP), DI
	MOVQ	$0, SI
	MOVQ	$0xffffffffff600000, AX
	CALL	AX

	MOVQ	8(SP), BX	// sec
	MOVQ	sec+0(FP), DI
	MOVQ	BX, (DI)

	MOVL	16(SP), BX	// usec
	MOVQ	usec+8(FP), DI
	MOVL	BX, (DI)
	RET

TEXT	rt_sigaction(SB),7,$0-32
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVQ	24(SP), DX
	MOVQ	32(SP), R10
	MOVL	$13, AX			// syscall entry
	SYSCALL
	RET

TEXT	sigtramp(SB),7,$64
	get_tls(BX)

	// save g
	MOVQ	g(BX), BP
	MOVQ	BP, 40(SP)

	// g = m->gsignal
	MOVQ	m(BX), BP
	MOVQ	m_gsignal(BP), BP
	MOVQ	BP, g(BX)

	MOVQ	DI, 0(SP)
	MOVQ	SI, 8(SP)
	MOVQ	DX, 16(SP)
	CALL	sighandler(SB)

	// restore g
	get_tls(BX)
	MOVQ	40(SP), BP
	MOVQ	BP, g(BX)
	RET

TEXT	sigignore(SB),7,$0
	RET

TEXT	sigreturn(SB),7,$0
	MOVL	$15, AX	// rt_sigreturn
	SYSCALL
	INT $3	// not reached

TEXT	·mmap(SB),7,$0
	MOVQ	8(SP), DI
	MOVQ	$0, SI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	28(SP), R10
	MOVL	32(SP), R8
	MOVL	36(SP), R9

	MOVL	$9, AX			// mmap
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	3(PC)
	NOTQ	AX
	INCQ	AX
	RET

TEXT	·munmap(SB),7,$0
	MOVQ	8(SP), DI
	MOVQ	16(SP), SI
	MOVQ	$11, AX	// munmap
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	2(PC)
	CALL	notok(SB)
	RET

TEXT	notok(SB),7,$0
	MOVQ	$0xf1, BP
	MOVQ	BP, (BP)
	RET

// int64 futex(int32 *uaddr, int32 op, int32 val,
//	struct timespec *timeout, int32 *uaddr2, int32 val2);
TEXT	futex(SB),7,$0
	MOVQ	8(SP), DI
	MOVL	16(SP), SI
	MOVL	20(SP), DX
	MOVQ	24(SP), R10
	MOVQ	32(SP), R8
	MOVL	40(SP), R9
	MOVL	$202, AX
	SYSCALL
	RET

// int64 clone(int32 flags, void *stack, M *m, G *g, void (*fn)(void));
TEXT	clone(SB),7,$0
	MOVL	flags+8(SP), DI
	MOVQ	stack+16(SP), SI

	// Copy m, g, fn off parent stack for use by child.
	// Careful: Linux system call clobbers CX and R11.
	MOVQ	mm+24(SP), R8
	MOVQ	gg+32(SP), R9
	MOVQ	fn+40(SP), R12

	MOVL	$56, AX
	SYSCALL

	// In parent, return.
	CMPQ	AX, $0
	JEQ	2(PC)
	RET
	
	// In child, on new stack.
	MOVQ	SI, SP
	
	// Initialize m->procid to Linux tid
	MOVL	$186, AX	// gettid
	SYSCALL
	MOVQ	AX, m_procid(R8)

	// Set FS to point at m->tls.
	LEAQ	m_tls(R8), DI
	CALL	settls(SB)

	// In child, set up new stack
	get_tls(CX)
	MOVQ	R8, m(CX)
	MOVQ	R9, g(CX)
	CALL	stackcheck(SB)

	// Call fn
	CALL	R12

	// It shouldn't return.  If it does, exit
	MOVL	$111, DI
	MOVL	$60, AX
	SYSCALL
	JMP	-3(PC)	// keep exiting

TEXT	sigaltstack(SB),7,$-8
	MOVQ	new+8(SP), DI
	MOVQ	old+16(SP), SI
	MOVQ	$131, AX
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	2(PC)
	CALL	notok(SB)
	RET

// set tls base to DI
TEXT settls(SB),7,$32
	ADDQ	$16, DI	// ELF wants to use -16(FS), -8(FS)

	MOVQ	DI, SI
	MOVQ	$0x1002, DI	// ARCH_SET_FS
	MOVQ	$158, AX	// arch_prctl
	SYSCALL
	CMPQ	AX, $0xfffffffffffff001
	JLS	2(PC)
	CALL	notok(SB)
	RET

