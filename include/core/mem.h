
extern  void*   malloc(ulong);
extern  void    free(void*);
extern  void*   mallocz(ulong, bool);
extern  void*   realloc(void*, ulong);
// less useful
//extern  ulong   msize(void*);
//extern  void*   calloc(ulong, ulong);
//extern  void*   mallocalign(ulong, ulong, long, ulong);

extern  void*   memset(void*, int, ulong);
extern  void*   memcpy(void*, void*, ulong);
extern  void*   memmove(void*, void*, ulong);
extern  int     memcmp(void*, void*, ulong);
extern  void*   memchr(void*, int, ulong);
// less useful
//extern  void*   memccpy(void*, void*, int, ulong);

// internals (useful for debugging)
// alt: in debug.h
extern  void    setmalloctag(void*, ulong);
extern  void    setrealloctag(void*, ulong);
extern  ulong   getmalloctag(void*);
extern  ulong   getrealloctag(void*);
extern  void*   malloctopoolblock(void*);
