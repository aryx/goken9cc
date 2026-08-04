
//extern  ulong   getfcr(void);
//extern  void    setfsr(ulong);
//extern  ulong   getfsr(void);
//extern  void    setfcr(ulong);
//
//extern  ulong   umuldiv(ulong, ulong, ulong);
//extern  long    muldiv(long, long, long);

// misc
//extern  double  charstod(int(*)(void*), void*);

// claude: cleanname() moved to os/path.h (its real home -- a Plan9
// namespace/path concept, not misc math) and implemented there; this
// file had a second, unrelated commented-out copy of the same
// declaration.
//extern  int     encodefmt(Fmt*);
//
//extern  int     getfields(char*, char**, int, int, char*);
//extern  int     gettokens(char *, char **, int, char *);
//
//extern  int     iounit(fdt);
//
