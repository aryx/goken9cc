// Feature test for the MIPS LL (load-linked), SC (store-conditional) and
// SYNC (memory barrier) instructions, ported to va/vc/vl from 9front:
//   10f1eeb7d v[al], libmach: add LL (load-linked) and SC (store-conditional)
//   ca2589a11 v[al]: add SYNC (memory barrier)
//   e7d099ac4 vl: LL operand order and scheduling
//   2eb28f43e vl: atomic instruction scheduling
// See plan9front.txt for the full list of credits.
//
// Before this port, va didn't even recognize "LL"/"SC"/"SYNC" as
// mnemonics (assembly fails to build at all), and vl had no opcode
// bits, optab entries, or scheduler awareness for them. This is what
// libc needs to implement spinlocks/mutexes on MIPS.
//
// This does a classic LL/SC compare-and-retry increment loop (the same
// idiom real lock code uses) on a word in .data, wrapped in SYNC
// barriers, then exit()s with the result. Getting the operand order
// wrong (SC's result overwrites its *source* register, see
// linkers/vl/sched.c's C_REG special case) or letting the linker's
// instruction scheduler reorder/duplicate the LL/SC pair (see
// linkers/vl/sched.c depend()'s "atomic instructions cannot pass"
// check) would make this loop over- or under-count, or hang.

TEXT _start(SB), $0
	MOVW	$setR30(SB), R30

	MOVW	$counter(SB), R5

	SYNC
retry:
	LL	(R5), R6
	ADDU	$1, R6, R6
	SC	R6, (R5)
	BEQ	R6, retry
	SYNC

	/* exit(counter) -- must be 11: started at 10, incremented once */
	MOVW	(R5), R4
	MOVW	$4001, R2           /* syscall = exit */
	SYSCALL

GLOBL	counter(SB), $4
DATA	counter(SB)/4, $10
