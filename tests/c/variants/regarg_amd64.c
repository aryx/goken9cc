// Regression test for 6c's REGARG=D_BP first-argument-in-register
// optimization silently corrupting calls into hand-written assembly
// like write(fd, buf, len): garg1() (compilers/6c/txt.c) computed the
// first eligible argument into BP, but none of this project's
// hand-written .s syscall stubs know to look there (they all expect
// the plain all-on-the-stack convention, like 8c). Nothing ever read
// or spilled BP, so fd ended up being whatever garbage was already on
// the stack. Fixed by disabling the optimization (REGARG=-1 in
// include/6.out.h), matching 8c and Go-era's own 6cg.
//
// Minimal repro: pass a non-constant fd (computed by a call, so 6c
// can't constant-fold the whole write() away) and confirm the bytes
// actually land on that fd, not on garbage.

extern int write(int fd, char *buf, int count);
extern void exit(int code);

int
stdout_fd(void)
{
	return 1;
}

void
main(void)
{
	write(stdout_fd(), "ok\n", 3);
	exit(0);
}
