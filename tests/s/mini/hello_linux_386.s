// see hello_linux_amd64.s for more information

// -------------------------------------------
// main procedure
// -------------------------------------------

TEXT    _start+0(SB), 7, $0

        // Allocate space for return address (CALL pushes return addr)
        SUBL    $8, SP         // make space: 8 bytes each for buf, len

        LEAL    msg(SB), AX     // get pointer to message
	MOVL    $1, 0(SP)       // fd = 1 (stdout)
        MOVL    AX, 4(SP)       // 
        MOVL    $14, 8(SP)      // 

        CALL    write(SB)

        ADDL    $8, SP         // clean up stack

        // syscall: exit(0)
        MOVL    $1, AX
        XORL    BX, BX
        INT     $0x80

// -------------------------------------------
// write(buf *byte, len int)
// -------------------------------------------
TEXT write(SB),7,$0
	MOVL	$4, AX		// syscall - write
	MOVL	4(SP),  BX
	MOVL	8(SP), CX
	MOVL	12(SP), DX
	INT	$0x80
	RET

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/6, $"orld\n"
GLOBL   msg(SB), $14
