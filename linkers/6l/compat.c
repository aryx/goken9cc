#include	"l.h"

/*
 * fake malloc
 *
 * claude: every block is prefixed with a hidden vlong holding its
 * requested size, so realloc() below can grow a block (needed by
 * lk/macho.c's machorebase(), which reallocs its offset array). The
 * header keeps the returned pointer 8-byte aligned since it is itself
 * 8 bytes and hunk only ever advances by multiples of 8.
 *
 * TODO: this whole gethunk/bump-arena allocator (here and in the other
 * kencc-lineage linkers' compat.c/sub.c/utils.c) is vestigial history:
 * these are short-lived, single-pass batch programs that allocate lots
 * of small Sym/Prog/Adr nodes and never free them individually, so a
 * bump allocator (O(1) per alloc, no free-list bookkeeping, everything
 * reclaimed at once on process exit) made sense on the original
 * resource-constrained Plan9 hardware. It predates having a reliable
 * hosted libc to lean on, unlike the Go-era src/cmd/6l/ld, which never
 * had this and just calls the real system malloc/realloc directly.
 * Nothing here actually needs a non-freeing arena today, so the plan
 * is to eventually delete it and call the real lib9/libc
 * malloc/realloc/free directly. If/when doing that:
 *   - free() below is a no-op today; a real free() will actually
 *     reclaim memory, so any code that keeps a pointer around after
 *     "freeing" it (relying on the arena's never-really-freed
 *     semantics) will start reading/writing freed memory -- audit
 *     every free() callsite in these linkers first.
 *   - the hidden size header and its pointer arithmetic (here and in
 *     realloc()) are only needed by this arena; drop them entirely
 *     once malloc/realloc call straight into libc, which already
 *     tracks block sizes itself.
 *   - hunk/nhunk/gethunk()/mysbrk() become dead code and can go too.
 */
void*
malloc(ulong n)
{
	vlong *p;
	ulong n2;

	n2 = n + sizeof(vlong);
	while(n2 & 7)
		n2++;
	while(nhunk < n2)
		gethunk();
	p = (vlong*)hunk;
	*p = n;
	nhunk -= n2;
	hunk += n2;
	return p+1;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(ulong m, ulong n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void *v, ulong n)
{
	void *p;
	vlong oldn;

	if(v == nil)
		return malloc(n);
	oldn = ((vlong*)v)[-1];
	if(n <= oldn)
		return v;
	p = malloc(n);
	memmove(p, v, oldn);
	return p;
}

void*
mysbrk(ulong size)
{
	return sbrk(size);
}

/* claude: lk/macho.c (shared with 7l) allocates its MachoLoad/MachoSect
 * arrays via halloc(); 7l has its own hunk-based halloc() in sub.c, but
 * that's functionally identical to our malloc() above, so just reuse it */
void*
halloc(ulong n)
{
	return malloc(n);
}

void
setmalloctag(void*, ulong)
{
}

//old: now in libc
//int
//fileexists(char *s)
//{
//	uchar dirbuf[400];
//
//	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
//	return stat(s, dirbuf, sizeof(dirbuf)) >= 0;
//}
