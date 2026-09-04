#include <signal.h>

void
handler(int n)
{
}

void
main(void)
{
	signal(SIGFPE, handler);
}
