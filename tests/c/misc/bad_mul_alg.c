int foo(int x) {
    // this caused an error with 8c (but not 5c):
    //   bad_mul_alg.c:2 bad mul alg
    return x * 129;
}
