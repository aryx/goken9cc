//TODO: merge once iar is fixed to handle .7 objects
#ifndef arm64
#pragma lib "libprint.a"
#endif

#include "minilibc.h"

void main() {
  printf("Hello World: %d\n", 42);
  exit(0);
}

// just to see the generated assembly code when using 6c/7c -S
void test() {
  write(0, "test\n", 5);
  exit(42);
}
