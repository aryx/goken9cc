//---------------------------------
// Entry and exit point
//---------------------------------

TEXT _main(SB), $0
	MOVW $setR30(SB), R30
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

//extern void write(uint32 fd, char* buf, /*size_t*/ int count);
TEXT write+0(SB), $0
	//the first arg is passed via R1

        MOVW    R1, R4
        MOVW    buf+4(FP), R5
        MOVW    count+8(FP), R6
        MOVW    $4004, R2          // syscall number 4 = sys_write
        SYSCALL
	RET
