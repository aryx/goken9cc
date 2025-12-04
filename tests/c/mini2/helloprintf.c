// if iar is not working for your arch you might want to
// uncomment the ifndef below to prevent the linker to
// load and link libprint.a like in ../mini/
//#ifndef arm64
#pragma lib "libprint.a"
//#endif

#include "minilibc.h"

int fact(int n) {
    if (n == 0) { return 1; }
    else { return n * fact (n - 1); }
}

void main() {
  printf("Hello World: %t\n", true);
  printf("Hello World: %x\n", 42);
  printf("Hello %s%s: %d\n", "Wor", "ld", 42);
  printf("Hello World: %d\n", 42);
  test();
  test_hello();
  printf("fact(5) = %d\n", fact(5));
  exit(0);
}

// just to see the generated assembly code when using 5c/6c/7c/... -S
void test_hello() {
  write(1, "test\n", 5);
  //exit(42);
}
