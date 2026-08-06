
typedef struct Tm Tm;
struct Tm {
    int sec;
    int min;
    int hour;

    int mday;
    int mon;
    int year;
    int wday;
    int yday;

    char    zone[4];
    int     tzoff;
};

extern  long    time(long*);
extern  vlong   nsec(void);

extern  Tm*     gmtime(long);
extern  Tm*     localtime(long);

extern  double  cputime(void);

extern  long    tm2sec(Tm*);

extern  char*   asctime(Tm*);
extern  char*   ctime(long);

// less useful?
//extern  long    times(long*);
//
//extern  void    cycles(uvlong*);    /* 64-bit value of the cycle counter if there is one, 0 if there isn't */


/* claude: alarm(ms) arms a ONE-SHOT timer ms milliseconds from now and
 * returns the milliseconds left on the previous one (0 if none was
 * pending, -1 on failure); alarm(0) cancels. On expiry the process gets
 * an "alarm" note, which -- as with any unhandled note -- kills it if
 * nothing has called notify().
 *
 * Backed by a real syscall on plan9 (syscall/os/plan9/svc_$cputype.s),
 * by setitimer(ITIMER_REAL) on linux and darwin (os/$GOOS/alarm.c), and
 * by a documented always--1 on windows, which has timers but no note
 * mechanism to deliver an expiry to (os/windows/alarm.c explains).
 *
 * Deliberately NOT accompanied by a general setitimer()/Itimer-shaped
 * call, even though this tree does extend the Plan9 API when there is a
 * reason to (include/os/proc.h's spawn()). spawn() earned its place by
 * being MORE portable than what it replaced: fork()+exec() cannot be
 * expressed on windows at all, so a new primitive was the only way to
 * cover all four GOOSes. A general interval timer would go the other
 * way -- plan9's kernel offers exactly this one-shot alarm and nothing
 * else, so the richer API could not be backed on the very GOOS whose
 * API this is, and would have to be emulated in userspace there. The
 * expressiveness actually missing (a REPEATING timer) is also the part
 * a caller can supply portably by re-arming from its own note handler.
 * If a real caller ever needs more, the raw setitimer stub is already
 * generated on linux and darwin (_syssetitimer in each .decl), so the
 * syscall plumbing is done and only the portable-surface question is
 * left open.
 */
extern	long	alarm(ulong);
extern	int	sleep(long); //less: could be void (ulong). 0 means yield.
