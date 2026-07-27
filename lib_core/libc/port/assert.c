#include <u.h>
#include <libc.h>

/* default implementation of the function pointer declared in
 * include/debugging/debug.h; assert(x) calls _assert("x") when x is false */
static void
__assert_fail(char *s)
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
	abort();
}

void (*_assert)(char*) = __assert_fail;
