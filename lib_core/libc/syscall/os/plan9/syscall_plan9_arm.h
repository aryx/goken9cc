/* Companion header for the (empty) decl-generated zsyscall_plan9_arm.c
 * -- see syscall_plan9_arm.decl's own comment for why this arch needs
 * no shared trampoline. read()/write()/exit() are hand-written here
 * (not raw syscalls themselves, so they don't belong in svc_arm.s)
 * instead of decl-generated, since none of them is a plain 1:1 syscall
 * wrapper mksyscall.sh's model supports:
 *
 * - read()/write() wrap the raw pread()/pwrite() syscalls (svc_arm.s)
 *   with a sentinel offset meaning "current file position" -- adapted
 *   from ~/principia-softwarica/lib_core/libc/9sys/{read,write}.c
 *   (real, working Plan9 libc code): `pread(fd, buf, n, -1LL)`.
 * - exit(int) adapts POSIX's int exit code onto Plan9's real exits(char*)
 *   syscall (status *string*, not exit code -- nil/empty means success,
 *   any non-empty string means failure), matching the POSIX-style
 *   exit(int) every other GOOS here already provides (include/os/
 *   proc.h's own `extern void exit(int);`). There's no faithful
 *   int->string mapping for a nonzero code (Plan9 has no such concept
 *   at all), so this collapses every nonzero code to the same generic
 *   error string -- a real, accepted loss of information, not an
 *   oversight.
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
