#pragma lib "libhello.a"

extern void xwrite(char*, int);
extern void xexit(void);

void
main(void)
{
	xwrite("hello, world\n", 13);
	xexit();
}
