// if iar is not working for your arch you might want to
// uncomment the ifndef below to prevent the linker to
// load and link libprint.a like in ../mini/
//#ifndef arm64
#pragma lib "libprint.a"
//#endif

#include "minilibc.h"

void main() {
  printf("Hello %s%s: %t\n", "Wor", "ld", true);
  printf("Hello %s%s: %x\n", "Wor", "ld", 42);
  printf("Hello %s%s: %d\n", "Wor", "ld", 42);
  printf("Hello World: %d\n", 42);


  printf("%f\n", NaN());
  printf("%f\n", 2.0); 
  printf("%f\n", 2.2); 
  // TODO: 2.0 is 2.000001 in arm but correctly 2 in amd64
  // TODO: 0.0 is incorrect too in arm, so is 0.333, lots of pbs
  printf("Hello World: %f\n", 0.0);
  printf("Hello World: %f\n", 3.0);

  test();

  exit(0);
}

// just to see the generated assembly code when using 5c/6c/7c/... -S
void test_hello() {
  write(1, "test\n", 5);
  exit(42);
}
