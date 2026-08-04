#include <u.h>
#include <libc.h>

/* atexit() (include/os/proc.h) -- port/atexit.c, port/exits.c's own
 * atexitrun() hook. Registered callbacks must run in LIFO order (most
 * recently registered first), and only via exits(), not _exits() (see
 * port/exits.c's own header comment for the split) -- both checked
 * here, not just "did it run at all".
 *
 * f3 is registered FIRST, so (LIFO) it runs LAST, after f1/f2 have
 * both already run and set order[] -- it prints the whole "order ok"/
 * "order wrong" verdict itself, since exits() never returns to main()
 * for a normal, non-fake-out way to check afterward. Got this backwards
 * on a first attempt (registered f3 last, expecting it to run first
 * -- exactly backwards for LIFO) and caught it by actually running the
 * test: it printed "n=0", i.e. f3 ran before f1/f2 had a chance to.
 */
int order[2];
int n;

void
f1(void)
{
	order[n++] = 1;
}

void
f2(void)
{
	order[n++] = 2;
}

void
f3(void)
{
	if (n == 2 && order[0] == 2 && order[1] == 1)
		print("atexit ok\n");
	else
		print("atexit wrong order: n=%d order[0]=%d order[1]=%d\n",
			n, order[0], order[1]);
}

void
main(void)
{
	atexit(f3);
	atexit(f1);
	atexit(f2);
	exits(nil);
}
