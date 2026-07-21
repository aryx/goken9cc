#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

// On Cygwin, siglongjmp() is a macro that does "&(env)" internally, so it
// needs an addressable sigjmp_buf lvalue, not just a pointer value; the
// dereferenced cast gives it one, pointing at the same buf storage. On
// every other platform siglongjmp() is a plain function and the original
// (void*) cast is left untouched.
#ifdef __CYGWIN__
#define SIGLONGJMP(buf, val) siglongjmp(*(sigjmp_buf*)(buf), (val))
#else
#define SIGLONGJMP(buf, val) siglongjmp((void*)(buf), (val))
#endif

void
p9longjmp(p9jmp_buf buf, int val)
{
	SIGLONGJMP(buf, val);
}

void
p9notejmp(void *x, p9jmp_buf buf, int val)
{
	USED(x);
	SIGLONGJMP(buf, val);
}

