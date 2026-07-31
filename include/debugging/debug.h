
#define assert(x)   do{ if(x) {} else _assert("x"); }while(0)

extern  void    (*_assert)(char*);

// on Plan 9 this is supposed to redirect to the debugger? or put
// process in broken state to be debugged?
extern	void	abort(void);

// useful for stack trace?
// alt: in reflect.h
extern  uintptr getcallerpc(void*);
