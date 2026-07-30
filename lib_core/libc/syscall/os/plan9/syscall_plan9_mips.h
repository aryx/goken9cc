/* Companion header for the (empty) decl-generated zsyscall_plan9_mips.c
 * -- see syscall_plan9_arm.h's identical comment for the full story
 * (read/write/exit hand-written here, not decl-generated).
 */
extern long pread(int fd, void *buf, long n, vlong offset);
extern long pwrite(int fd, void *buf, long n, vlong offset);
extern void exits(char *msg);

long
read(int fd, void *buf, long n)
{
	return pread(fd, buf, n, -1LL);
}

long
write(int fd, void *buf, long n)
{
	return pwrite(fd, buf, n, -1LL);
}

void
exit(int code)
{
	if (code == 0)
		exits(0);
	else
		exits("error");
}
