#pragma lib "libmini.a"

extern void xwrite(char*, int);
extern void xexit(void);

void main() {
  xwrite("Hello C World\n", 14);
  xexit();
}
