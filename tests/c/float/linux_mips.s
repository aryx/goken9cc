//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	MOVW $setR30(SB), R30

	// claude: vc reuses F24/F26/F28/F30 as hardwired 0.0/0.5/1.0/2.0
	// constants for common float literals (see compilers/vc/txt.c
	// gmove()) instead of loading them from memory. On real hardware
	// there is no such thing as a hardwired MIPS float register (unlike
	// ARM's FPA, which has actual silicon-wired constants), so the
	// kernel must initialize them once at boot -- see plan9-contrib
	// sys/src/9/loongson/l.s. Without this, every "small" float
	// constant (0.0, 0.5, 1.0, 2.0, 1.5, 2.5, 3.0, -1.0, -2.0, ...)
	// silently reads as whatever garbage F24-F30 start with (0 under
	// qemu-mips), which is why floats came out zeroed/looping.
	MOVD	$0.5, F26
	SUBD	F26, F26, F24
	ADDD	F26, F26, F28
	ADDD	F28, F28, F30

	JAL main(SB)

//extern void exit(uint32);
TEXT    exit+0(SB), $0

	MOVW    R1, R4              /* exit code                */
        MOVW    $4001, R2           /* syscall = exit           */
        SYSCALL
	RET // never reached

//---------------------------------
// Basic functions
//---------------------------------

//extern void	panic(int32);
TEXT    panic+0(SB), $0
	MOVW    R1, R4              /* exit code                */
        MOVW    $4001, R2           /* syscall = exit           */
        SYSCALL
	RET // never reached

TEXT    abort+0(SB), $0
	MOVW    $3, R4              /* exit code                */
        MOVW    $4001, R2           /* syscall = exit           */
        SYSCALL
	RET // never reached


//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT write+0(SB), $0
	//the first arg is passed via R1

        MOVW    R1, R4
        MOVW    buf+4(FP), R5
        MOVW    count+8(FP), R6
        MOVW    $4004, R2          // syscall number 4 = sys_write
        SYSCALL
	RET
