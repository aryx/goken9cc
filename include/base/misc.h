
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

// claude: needed by mk's own shprint.c -- port/getfields.c, ported
// from BOOT/lib9/getfields.c. Found self-hosting mk/ with goken's own
// compiler+libc instead of the host bootstrap gcc+lib9. Splits str IN
// PLACE (nul-ing each separator rune found in set) and fills args[]
// with pointers into str, up to max fields; if mflag is set, runs of
// consecutive separators collapse into one split point instead of
// producing empty fields between them.
extern  int     getfields(char*, char**, int, int, char*);
//extern  int     gettokens(char *, char **, int, char *);
//
//extern  int     iounit(fdt);
//
