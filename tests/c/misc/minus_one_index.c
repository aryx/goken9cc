void foo() {
    char *s;

    // 8c -S should generate some
    // MOVB	$0,-1(DX)
    // for the instruction above but because of some long vs int32
    // bug it can also generate
    // MOVL	DX,AX
	// ADDL	$4294967295,AX
    // MOVB	$0,(AX)


    s[-1] = 0;
}

// bug originally in strncat.c in:
//char*
//strncat(char *s1, char *s2, long n)
//{
//    char *os1;
//
//    os1 = s1;
//    while(*s1++)
//        ;
//    s1--;
//    while(*s1++ = *s2++)
//        if(--n < 0) {
//            s1[-1] = 0;
//            break;
//        }
//    return os1;
//}
