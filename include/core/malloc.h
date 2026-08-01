
extern  void*   malloc(ulong);
extern  void    free(void*);
extern  void*   mallocz(ulong, bool);
extern  void*   realloc(void*, ulong);
extern  void*   calloc(ulong, ulong);
// less useful
//extern  ulong   msize(void*);
//extern  void*   mallocalign(ulong, ulong, long, ulong);

// internals (useful for debugging)
// alt: in debug.h
extern  void    setmalloctag(void*, ulong);
extern  void    setrealloctag(void*, ulong);
extern  ulong   getmalloctag(void*);
extern  ulong   getrealloctag(void*);
extern  void*   malloctopoolblock(void*);
