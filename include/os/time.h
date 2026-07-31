
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

// less useful?
//extern  char*   asctime(Tm*);
//extern  char*   ctime(long);
//extern  long    times(long*);
//
//extern  void    cycles(uvlong*);    /* 64-bit value of the cycle counter if there is one, 0 if there isn't */


extern	long	alarm(ulong);
extern	int	sleep(long); //less: could be void (ulong). 0 means yield.
