// Linux m68k runtime stubs for the mini2 helloprintf test.
//
// Same conventions already established and verified by tests/c/mini's
// start_m68k.s/xwrite_m68k.s/xexit_m68k.s and tests/s/hello_arch/
// hello_linux_m68k.s (see those files' own comments, and docs/
// claude_notes/notes_arch_m68k.txt): A6 is the static base (bootstrap
// via a6base before any sym(SB) reference), and 2c passes ALL args on
// the stack (no REGARG register, unlike kc/qc/...) -- '2c -S' on a
// hand-written 3-arg function confirms fd/buf/count land at
// fd+0(FP)/buf+4(FP)/count+8(FP), the same FP-relative convention
// xwrite_m68k.s's own count+4(FP) already uses for its 2nd (and only
// stack) arg. No register save/restore dance needed around the syscall
// trap the way sparc's linux_sparc.s needs (m68k's real hardware d0 is
// both the syscall-number/return register AND already what 2c expects
// its own return value in -- see xwrite_m68k.s's own comment).

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

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT write+0(SB), $0
	MOVL	fd+0(FP), R1      // fd
	MOVL	buf+4(FP), R2     // buf
	MOVL	count+8(FP), R3   // count
	MOVL	$4, R0            // syscall number = write (4)
	TRAP	$0
	RTS
