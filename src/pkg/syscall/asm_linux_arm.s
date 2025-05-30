// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

//
// System calls for arm, Linux
//

// TODO(kaib): handle error returns

// func Syscall(syscall uintptr, a1, a2, a3 uintptr) (r1, r2, err uintptr);

TEXT	·Syscall(SB),7,$0
	BL		runtime·entersyscall(SB)
	MOVW	4(SP), R7
	MOVW	8(SP), R0
	MOVW	12(SP), R1
	MOVW	16(SP), R2
	SWI		$0
	MOVW	$0xfffff001, R1
	CMP		R1, R0
	BLS		ok
	MOVW	$-1, R1
	MOVW	R1, 20(SP)	// r1
	MOVW	$0, R2
	MOVW	R2, 24(SP)	// r2
	RSB		$0, R0, R0
	MOVW	R0, 28(SP)	// errno
	BL		runtime·exitsyscall(SB)
	RET
ok:
	MOVW	R0, 20(SP) // r1
	MOVW	$0, R0
	MOVW	R0, 24(SP)	// r2
	MOVW	R0, 28(SP)	// errno
	BL		runtime·exitsyscall(SB)
	RET

// func Syscall6(trap uintptr, a1, a2, a3, a4, a5, a6 uintptr) (r1, r2, err uintptr);
// Actually Syscall5 but the rest of the code expects it to be named Syscall6.
TEXT	·Syscall6(SB),7,$0
	BL		runtime·entersyscall(SB)
	MOVW	4(SP), R7	// syscall entry
	MOVW	8(SP), R0
	MOVW	12(SP), R1
	MOVW	16(SP), R2
	MOVW	20(SP), R3
	MOVW	24(SP), R4
	MOVW	28(SP), R5
	SWI		$0
	MOVW	$0xfffff001, R1
	CMP		R1, R0
	BLS		ok6
	MOVW	$-1, R1
	MOVW	R1, 32(SP)	// r1
	MOVW	$0, R2
	MOVW	R2, 36(SP)	// r2
	RSB		$0, R0, R0
	MOVW	R0, 40(SP)	// errno
	BL		runtime·exitsyscall(SB)
	RET
ok6:
	MOVW	R0, 32(SP) // r1
	MOVW	$0, R0
	MOVW	R0, 36(SP)	// r2
	MOVW	R0, 40(SP)	// errno
	BL		runtime·exitsyscall(SB)
	RET

// func RawSyscall(trap uintptr, a1, a2, a3 uintptr) (r1, r2, err uintptr);
TEXT ·RawSyscall(SB),7,$0
	MOVW	4(SP), R7	// syscall entry
	MOVW	8(SP), R0
	MOVW	12(SP), R1
	MOVW	16(SP), R2
	SWI		$0
	MOVW	$0xfffff001, R1
	CMP		R1, R0
	BLS		ok1
	MOVW	$-1, R1
	MOVW	R1, 20(SP)	// r1
	MOVW	$0, R2
	MOVW	R2, 24(SP)	// r2
	RSB		$0, R0, R0
	MOVW	R0, 28(SP)	// errno
	RET
ok1:
	MOVW	R0, 20(SP) // r1
	MOVW	$0, R0
	MOVW	R0, 24(SP)	// r2
	MOVW	R0, 28(SP)	// errno
	RET
