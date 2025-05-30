// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

TEXT	_rt0_386_plan9(SB),7, $0
	MOVL	AX, _tos(SB)
	
	// move arguments down to make room for
	// m and g at top of stack, right before Tos.
	MOVL	SP, SI
	SUBL	$8, SP
	MOVL	SP, DI
		
	MOVL	AX, CX
	SUBL	SI, CX
	CLD
	REP; MOVSB
	
	// adjust argv
	SUBL	SI, DI
	MOVL	newargc+0(SP), CX
	LEAL	newargv+4(SP), BP
argv_fix:
	ADDL	DI, 0(BP)
	ADDL	$4, BP
	LOOP	argv_fix
	
	JMP	_rt0_386(SB)

DATA  isplan9+0(SB)/4, $1
GLOBL isplan9(SB), $4
GLOBL _tos(SB), $4
