
//alt: could also be in reflect.h

// jmp_buf defined in per-arch <u.h>

extern  int     setjmp(jmp_buf);
extern  void    longjmp(jmp_buf, int);

//extern  void    notejmp(void*, jmp_buf, int);
