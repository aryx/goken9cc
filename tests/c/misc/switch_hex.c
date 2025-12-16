
enum {
    ARENA_MAGIC = 0xC0A1E5CE+1,
    ARENATAIL_MAGIC = 0xEC5E1A0C+1,
};
enum {
    ALLOC_MAGIC = 0x0A110C09,
    UNALLOC_MAGIC = 0xCAB00D1E+1,
};

// the compilation of the switch had different order with -m32 vs -m64
// before. In one case with 8c -S we had (with old -m64):
// 
//	TEXT	blockcheck+0(SB),0,$0
//	JMP	,4(PC)
//	JMP	,9(PC)
//	JMP	,-1(PC)
//	JMP	,-2(PC)
//	MOVL	$1,AX
//	CMPL	AX,$168889353
//	JEQ	,-3(PC)
//	CMPL	AX,$-329377267
//	JEQ	,-6(PC)
//	JMP	,-8(PC)
//	RET	,
//	END	,
//
// and in the other (with -m32)
//
//	TEXT	blockcheck+0(SB),0,$0
//	JMP	,4(PC)
//	JMP	,9(PC)
//	JMP	,-1(PC)
//	JMP	,-2(PC)
//	MOVL	$1,AX
//	CMPL	AX,$-329377267
//	JEQ	,-4(PC)
//	CMPL	AX,$168889353
//	JEQ	,-5(PC)
//	JMP	,-8(PC)
//	RET	,
//	END	,
//
// leading to different .8 object files for example on lib_core/libc/port/pool.8

void
blockcheck()
{
    switch(1) {
    case ARENATAIL_MAGIC:
        break;
    case ALLOC_MAGIC:
        break;
    }
}
