// -------------------------------------------
// main procedure
// -------------------------------------------
TEXT _start(SB), $0

	MOVW    $1, R0              /* fd = 1 (stdout) */
        MOVW    $msg(SB), R1        /* buf = &msg */
        MOVW    $13, R2          /* count = len */
        MOVW    $4, R7              /* syscall 4 = sys_write */
        SWI     $0

	// this seems to work with qemu-armhf but not when running
        // from my Ampere Altra :(
	MOVF	$0.0e+00,F0

	/* exit(0) */
        MOVW    $0, R0              /* exit code */
        MOVW    $1, R7              /* syscall 1 = sys_exit */
        SWI     $0

// for 5l (not 5l_)
TEXT 	_sfloat+0(SB), $0
	MOVW    $1, R0
        MOVW    $msg_sfloat(SB), R1
        MOVW    $8, R2
        MOVW    $4, R7
        SWI     $0
	RET

GLOBL   msg(SB), $13
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"

GLOBL   msg_sfloat(SB), $8
DATA    msg_sfloat+0(SB)/8, $"sfloat\n\n"
