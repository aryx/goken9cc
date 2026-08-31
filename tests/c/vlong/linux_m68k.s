// Linux m68k runtime stubs for the vlong helloprintf test.
//
// Same shape as tests/c/mini2/linux_m68k.s (see that file's own
// comment, and docs/claude_notes/notes_arch_m68k.txt) plus one extra
// stub, abort(), referenced by vlrt.c on an internal consistency
// error.

//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	MOVL $a6base(SB), A6
	BSR main(SB)

//extern void exit(uint32);
TEXT exit+0(SB), $0
	MOVL	status+0(FP), R1  // status (only arg, on the stack)
	MOVL	$1, R0            // syscall number = exit (1)
	TRAP	$0
	RTS                       // not reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void panic(int32);
TEXT panic+0(SB), $0
	MOVL	status+0(FP), R1  // status (only arg, on the stack)
	MOVL	$1, R0            // syscall number = exit (1)
	TRAP	$0
	RTS                       // not reached

//extern void abort(void); // referenced by vlrt.c
TEXT abort+0(SB), $0
	MOVL	$3, R1            // exit code = 3
	MOVL	$1, R0            // syscall number = exit (1)
	TRAP	$0
	RTS                       // not reached

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT write+0(SB), $0
	MOVL	fd+0(FP), R1      // fd
	MOVL	buf+4(FP), R2     // buf
	MOVL	count+8(FP), R3   // count
	MOVL	$4, R0            // syscall number = write (4)
	TRAP	$0
	RTS
