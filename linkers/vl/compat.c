#include	"l.h"

/*
 * fake malloc
 */
void*
malloc(uint32 n)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(uint32 m, uint32 n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void*, uint32)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void*
mysbrk(uint32 size)
{
	return sbrk(size);
}

void
setmalloctag(void *v, uint32 pc)
{
	USED(v, pc);
}
