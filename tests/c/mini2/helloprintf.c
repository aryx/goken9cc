#pragma lib "libprint.a"

typedef	signed char		int8;

extern void printf(int8 *s, ...);
extern void exit(int);

void main() {
  printf("Hello World: %d\n", 42);
  exit(0);
}
