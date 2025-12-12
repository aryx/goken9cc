
#define VLONG_MAX    ~(1LL<<63)

typedef long long vlong;

vlong test() {
    // 8c -S should generate some
    //TEXT	test+0(SB),0,$0
	//MOVL	.ret+0(FP),AX
	//MOVL	$-1,(AX)
	//MOVL	$2147483647,4(AX)

    // but instead was generating
    //TEXT	test+0(SB),0,$0
	//MOVL	.ret+0(FP),AX
	//MOVL	$9223372036854775807,(AX)
	//MOVL	$2147483647,4(AX)
    
    return VLONG_MAX;
}
