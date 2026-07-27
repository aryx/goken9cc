#include <u.h>
#include <libc.h>

void
main(void)
{
	print("hello from libc.a: %d + %d = %d\n", 2, 2, 2+2);
	exit(0);
}
