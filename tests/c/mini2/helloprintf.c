//TODO: merge once iar is fixed to handle .7 objects
#ifndef arm64
#pragma lib "libprint.a"
#endif

#include "minilibc.h"

void main() {
  printf("Hello %s%s: %t\n", "Wor", "ld", true);
  printf("Hello %s%s: %x\n", "Wor", "ld", 42);
  printf("Hello %s%s: %d\n", "Wor", "ld", 42);
  printf("Hello World: %d\n", 42);
  // TODO: cause "Illegal instruction" with 5l_
  //TODO: printf("Hello World: %f\n", 0);
  //TODO: printf("Hello World: %f\n", 0.3333);
  exit(0);
}

// just to see the generated assembly code when using 6c/7c -S
void test() {
  write(1, "test\n", 5);
  exit(42);
}
