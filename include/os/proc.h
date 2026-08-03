
// Plan 9 specific
// exits() is the libc exit that performs some cleanup (and handle atexit)
// while _exits() is the syscall that is more abrupt
// TODO? still the case in goken libc? maybe reverted now
// alt: move under os/plan9/proc.h
extern	void	_exits(char*);
extern  void    exits(char*);

// in <stdlib.h>, with _exit() in <unistd.h>
// plain POSIX-style exit(int), alongside Plan9's own exits(char*)/_exits(char*)
extern	void	exit(int);

extern  int     atexit(void(*)(void));

// Plan 9 specific (move in os/plan9/proc.h?)
extern	int	rfork(int);
// in <unistd.h>
extern	int	fork(void);

// alt: Plan 9 specific, no wait message in Unix, move under os/plan9/wait.h?
/* keep /sys/src/ape/lib/ap/plan9/sys9.h in sync with this -rsc */
typedef struct Waitmsg Waitmsg;
struct Waitmsg {
 int	pid;		/* of loved one */
 ulong	time[3];	/* of loved one & descendants */
 // ref_own?<string>?
 char	*msg;
};

extern	Waitmsg* wait(void);
//extern	Waitmsg*	waitfor(pidt);
//extern	Waitmsg*	waitnohang(void);

extern	int	await(char*, int);
//extern	int	awaitfor(int, char*, int);
//extern	int	awaitnohang(char*, int);

extern	int	exec(char*, char*[]);

extern	int	execl(char*, ...);

extern	int	waitpid(void);

// in <unistd.h>
extern	int	getpid(void);
extern	int	getppid(void);
