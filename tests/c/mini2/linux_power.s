// PowerPC Linux runtime stubs for the mini2 helloprintf test.
//
// Same shape as tests/c/vlong/linux_power.s (see that file's own
// comment for the R3-arg/R2-SB/-0/stack-offset background); write()
// here has the identical 3-arg signature so it reuses the same
// buf+4(FP)/count+8(FP) offsets. No abort() here (unlike vlong's
// stub): nothing in this test links vlrt.c, which is the only thing
// that references it.
//
// PowerPC Linux syscall ABI: r0 = syscall number, r3-r5 = args.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	// static base (gp = R2) so the $.string(SB) refs in main resolve
	MOVW	$setSB(SB), R2
	BL	main(SB)

//extern void exit(uint32);
TEXT	exit+0(SB), $0
	// exit code already arrives in R3, which is also where the exit
	// syscall wants it -- nothing to move.
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT	panic+0(SB), $0
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT	write+0(SB), $0
	// fd already arrives in R3, same register the write syscall wants
	// it in -- nothing to move.
	MOVW	buf+4(FP), R4   // buf
	MOVW	count+8(FP), R5 // count
	MOVW	$4, R0          // syscall number = write (4)
	SYSCALL
	RETURN
