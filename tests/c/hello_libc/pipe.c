#include <u.h>
#include <libc.h>

/* pipe() alone (include/os/ipc.h) -- split out of fork.c (which
 * exercises fork()/exec()/wait()/pipe() together, the shape
 * compilers/pcc/pcc.c actually needs) specifically so this one call
 * has a test that can run on Plan9 under 5i/vi: machines/5i/vi's own
 * sysrfork()/sysexec()/sysawait() are still TODO stubs there (a real,
 * separate emulator gap -- see todo.org), so fork.exe cannot run on
 * plan9 even though it links, but PIPE genuinely IS implemented in
 * both emulators (machines/5i/syscall_posix.c's and machines/vi/
 * syscall.c's own syspipe(), which just calls the host's real pipe()).
 *
 * No fork() needed to test pipe() meaningfully: write into fd[1] and
 * read back from fd[0] in the SAME process, the same "loopback"
 * approach io.c already uses for read/write. This proves the two ends
 * are really connected (unlike two fds that merely fail to error),
 * just without fork.c's cross-process angle.
 */
void
main(void)
{
	int fd[2];
	char buf[64];
	long n;

	if(pipe(fd) < 0){
		print("pipe failed\n");
		exit(1);
	}
	n = write(fd[1], "hello pipe\n", 11);
	if(n != 11){
		print("short write\n");
		exit(1);
	}
	n = read(fd[0], buf, sizeof buf);
	if(n <= 0){
		print("pipe read failed\n");
		exit(1);
	}
	write(1, buf, n);
	close(fd[0]);
	close(fd[1]);

	print("pipe ok\n");
	exit(0);
}
