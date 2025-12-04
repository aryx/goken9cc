#pragma lib "libprint.a"
#include "../mini2/minilibc.h"

void main() {
  printf("Hello World: %t\n", true);
  printf("Hello World: %x\n", 42);
  printf("Hello %s%s: %d\n", "Wor", "ld", 42);
  printf("Hello World: %d\n", 42);
  test_hello();
  exit(0);
}

// just to see the generated assembly code when using 5c/6c/7c/... -S
void test_hello() {
  write(1, "test\n", 5);
  //exit(42);
}
