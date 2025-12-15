
#define VLONG_MAX    ~(1LL<<63)

typedef long long vlong;

vlong test() {
    // 8c -S should display some
    //TEXT	test+0(SB),0,$0
	//MOVL	.ret+0(FP),AX
	//MOVL	$-1,(AX)
	//MOVL	$2147483647,4(AX)

    // but instead was displaying
    //TEXT	test+0(SB),0,$0
	//MOVL	.ret+0(FP),AX
	//MOVL	$9223372036854775807,(AX)
	//MOVL	$2147483647,4(AX)

    // because of some %ld and long instead of %d and int32
    // for Adr offset in gc.h
    
    return VLONG_MAX;
}
