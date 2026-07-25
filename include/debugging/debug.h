
#define assert(x)   do{ if(x) {} else _assert("x"); }while(0)

extern  void    (*_assert)(char*);

extern	void	abort(void);

// useful for stack trace?
// alt: in reflect.h
extern  uintptr getcallerpc(void*);

