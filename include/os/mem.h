
// in <unistd.h>
extern  int brk(void*);
extern	void*	sbrk(ulong);

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
// alt: in reflect.h
extern  char    end[];

