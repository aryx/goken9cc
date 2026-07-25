
/* keep /sys/src/ape/lib/ap/plan9/sys9.h in sync with this -rsc */
struct Waitmsg {
 int	pid;		/* of loved one */
 ulong	time[3];	/* of loved one & descendants */
 // ref_own?<string>?
 char	*msg;
};

extern	void	_exits(char*);

extern	int	rfork(int);
extern	int	exec(char*, char*[]);
extern	int	await(char*, int);

extern  void    exits(char*);
extern  int     atexit(void(*)(void));

extern	int	fork(void);
extern	int	execl(char*, ...);
extern	Waitmsg*wait(void);
extern	int	waitpid(void);

extern	int	getpid(void);
extern	int	getppid(void);
