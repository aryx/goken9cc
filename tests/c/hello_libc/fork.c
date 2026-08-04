#include <u.h>
#include <libc.h>

/* fork()/exec()/wait()/pipe() -- Tier 4 process control
 * (docs/claude_notes/plan_syscalls.txt), the first tier whose calls
 * this libc did not already have a portable Plan9-shaped wrapper for
 * on any GOOS before this round. All four together are exactly what
 * compilers/pcc/pcc.c's own doexec()/dopipe() need to spawn and pipe
 * between subcommands -- the actual self-hosting milestone the plan
 * doc names this tier for -- so this test exercises them the same way
 * pcc.c does: fork+wait, fork+exec+wait, and pipe+fork+wait.
 *
 * The exec() half re-execs THIS SAME BINARY rather than depending on
 * some other program being present: exec()/execve() do no $PATH search
 * at all, matching real Plan9 exec(2) (see port/exec.c's own comment),
 * so there is nothing else in this test tree guaranteed to be at a
 * known, arch-matching path. "child" is passed as an extra argument so
 * the re-exec'd process can tell itself apart from a normal run.
 * argv[0] (what this process was invoked as) is what gets re-exec'd --
 * args.c's own comment explains why that's usable here even though its
 * exact value is never printed: it differs across environments (qemu
 * vs native, different tmp dirs), but is always a valid path back to
 * this same binary since cwd is unchanged across fork()/exec().
 */
void
main(int argc, char *argv[])
{
	Waitmsg *w;
	int pid, fd[2];
	char buf[64];
	long n;

	/* re-exec'd child branch: print a fixed line and exit cleanly. */
	if(argc > 1 && strcmp(argv[1], "child") == 0){
		print("child alive\n");
		exit(0);
	}

	/* 1: fork() alone. The child must see 0 and exit with a distinct,
	 * checkable code; the parent must see the child's own pid (not 0,
	 * not -1) and wait() must report that exact pid and exit code back
	 * -- the two things a fork()+wait() pair promises that a bare
	 * "did it return without erroring" check would miss entirely.
	 */
	pid = fork();
	if(pid < 0){
		print("fork failed\n");
		exit(1);
	}
	if(pid == 0)
		exit(42);
	w = wait();
	if(w == nil){
		print("wait failed\n");
		exit(1);
	}
	if(w->pid != pid){
		print("wait returned the wrong pid\n");
		exit(1);
	}
	if(strcmp(w->msg, "42") != 0){
		print("wait reported wrong exit status: %s\n", w->msg);
		exit(1);
	}
	free(w);
	print("fork+wait ok\n");

	/* 2: fork()+exec()+wait(), the exact shape compilers/pcc/pcc.c's
	 * doexec() uses. A successful exec() never returns, so the
	 * "exec failed" print below is reached only on real failure.
	 */
	pid = fork();
	if(pid < 0){
		print("fork (for exec) failed\n");
		exit(1);
	}
	if(pid == 0){
		char *cargv[3];

		cargv[0] = argv[0];
		cargv[1] = "child";
		cargv[2] = nil;
		exec(argv[0], cargv);
		print("exec failed\n");
		exit(1);
	}
	w = wait();
	if(w == nil){
		print("wait (for exec) failed\n");
		exit(1);
	}
	if(w->msg[0]){
		print("exec'd child reported failure: %s\n", w->msg);
		exit(1);
	}
	free(w);
	print("fork+exec+wait ok\n");

	/* 3: pipe()+fork()+wait(), the exact shape dopipe() uses. The
	 * parent reads what the child wrote to confirm the two ends are
	 * genuinely connected, not just two fds that happen not to error.
	 */
	if(pipe(fd) < 0){
		print("pipe failed\n");
		exit(1);
	}
	pid = fork();
	if(pid < 0){
		print("fork (for pipe) failed\n");
		exit(1);
	}
	if(pid == 0){
		close(fd[0]);
		write(fd[1], "hello from child\n", 17);
		close(fd[1]);
		exit(0);
	}
	close(fd[1]);
	n = read(fd[0], buf, sizeof buf);
	if(n <= 0){
		print("pipe read failed\n");
		exit(1);
	}
	write(1, buf, n);
	close(fd[0]);
	w = wait();
	if(w == nil){
		print("wait (for pipe) failed\n");
		exit(1);
	}
	free(w);
	print("pipe+fork+wait ok\n");

	exit(0);
}
