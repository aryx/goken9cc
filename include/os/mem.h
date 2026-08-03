
// "Give me a pointer to a big area of memory"
extern	void*	sbrk(ulong);

// in <unistd.h>
// Do not use brk() because it is not portable and does not really work
// on macOS and Windows. Use sbrk() instead (or malloc()/free()).
extern  int brk(void*);

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
// alt: in reflect.h
extern  char    end[];
