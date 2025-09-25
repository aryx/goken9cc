
TEXT    exit+0(SB), 7, $0
        MOVW    status+0(FP), R0
        MOVW    $1, R7          // syscall number 1 = sys_exit
        SWI     $0
	RET // never reached

TEXT    panic+0(SB), 7, $0
        MOVW    status+0(FP), R0
        MOVW    $1, R7          // syscall number 1 = sys_exit
        SWI     $0
	RET // never reached

TEXT write+0(SB), 7, $0
        MOVW    fd+0(FP), R0
        MOVW    buf+4(FP), R1
        MOVW    count+8(FP), R2
        MOVW    $4, R7          // syscall number 4 = sys_write
        SWI     $0
	RET

// from 9front/.../libc/arm/getcallerpc.s
TEXT ·getcallerpc(SB), 1, $-4
	MOVW	0(R13), R0
	RET

// no arm instructions for those operations so must be
// provided as "builtins"
TEXT 	_div+0(SB), 7, $0
	RET
TEXT 	_divu+0(SB), 7, $0
	RET
TEXT 	_mod+0(SB), 7, $0
	RET
TEXT 	_modu+0(SB), 7, $0
	RET

//Float and 64bits stuff
//TEXT 	_si2v+0(SB), 7, $0
//	RET
//TEXT 	_ui2v+0(SB), 7, $0
//	RET
//TEXT 	_v2si+0(SB), 7, $0
//	RET
//TEXT 	_v2sl+0(SB), 7, $0
//	RET
//TEXT 	_v2uc+0(SB), 7, $0
//	RET
//TEXT 	_v2ul+0(SB), 7, $0
//	RET
//TEXT 	_v2ui+0(SB), 7, $0
//	RET
//TEXT 	_p2v+0(SB), 7, $0
//	RET
//
//TEXT 	_modvu+0(SB), 7, $0
//	RET
//TEXT 	_divvu+0(SB), 7, $0
//	RET
//
//TEXT 	_addv+0(SB), 7, $0
//	RET
//TEXT 	_subv+0(SB), 7, $0
//	RET
//TEXT 	_andv+0(SB), 7, $0
//	RET
//TEXT 	_xorv+0(SB), 7, $0
//	RET
//TEXT 	_orv+0(SB), 7, $0
//	RET
//TEXT 	_eqv+0(SB), 7, $0
//	RET
//TEXT 	_lshv+0(SB), 7, $0
//	RET
//TEXT 	_rshlv+0(SB), 7, $0
//	RET
//
//
//TEXT 	_lov+0(SB), 7, $0
//	RET
//TEXT 	_ltv+0(SB), 7, $0
//	RET
//TEXT 	_vasop+0(SB), 7, $0
//	RET
//TEXT 	_hiv+0(SB), 7, $0
//	RET
//TEXT 	_sfloat+0(SB), 7, $0
//	RET
