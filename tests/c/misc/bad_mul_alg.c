int foo(int x) {
    // this caused an error with 8c (but not 5c) and with 128 below it's fine
    //   bad_mul_alg.c:2 bad mul alg
    return x * 129;
}
