
// Plan 9 specific
// exits() is the libc exit that performs some cleanup (and handle atexit())
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
// claude: rfork(2)'s flag bits (Tier 4 process control, docs/claude_notes/
// plan_syscalls.txt). Ported from principia's include/core/syscall.h
// "enum Rfork_flags" (the authoritative real-Plan9 values, not
// guessed) -- os/plan9/fork.c's fork() is built directly on RFPROC|
// RFFDG|RFREND, the same combination principia's own 9sys/fork.c uses.
enum {
	RFNAMEG  = (1<<0),
	RFENVG   = (1<<1),
	RFFDG    = (1<<2),
	RFNOTEG  = (1<<3),
	RFPROC   = (1<<4),
	RFMEM    = (1<<5),
	RFNOWAIT = (1<<6),
	RFCNAMEG = (1<<10),
	RFCENVG  = (1<<11),
	RFCFDG   = (1<<12),
	RFREND   = (1<<13),
	RFNOMNT  = (1<<14),
};
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

// claude: spawn() -- a portable process-spawn primitive covering the
// one thing rfork()/fork()+dup()+exec() cannot express uniformly:
// there is no fork() on windows at all, only the atomic CreateProcess.
// See docs/claude_notes/notes_libc_api_design.txt's "spawn(): a
// portable process-spawn primitive" section for the full design story
// (and real precedent: POSIX's own posix_spawn(), libuv's uv_spawn(),
// Rust's Command::spawn(), ...). Starts path/argv with its stdin/
// stdout/stderr connected to fdin/fdout/fderr (-1 meaning "leave mine
// as they are"); returns immediately with a pid wait()/waitpid() can
// later be given, or -1 on failure. Does NOT feed fdin or drain fdout
// itself -- same division of labor as fork()+dup()+exec() already has:
// build a pipe, hand one end here, read/write the other yourself.
extern	int	spawn(char*, char**, fdt, fdt, fdt);

// in <unistd.h>
extern	int	getpid(void);
extern	int	getppid(void);

// process id type. I like types! similar to os/file.h fdt.
typedef int pidt;
