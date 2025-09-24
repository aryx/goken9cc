//TODO: fix iar to handle .7 objects
#ifndef arm64
#pragma lib "libmini.a"
#endif

extern void xwrite(char*, int);
extern void xexit(void);

void main() {
  xwrite("Hello C World\n", 14);
  xexit();
}
