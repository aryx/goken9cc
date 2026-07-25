
extern  int brk(void*);

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
// alt: in reflect.h
extern  char    end[];

extern	void*	sbrk(ulong);
