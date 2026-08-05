#include	"l.h"

/*
 * claude: same fix as 8l/compat.c (docs/claude_notes/notes_os_macos.txt's
 * "gethunk()/sbrk() OOM bug" section) -- this whole fake malloc/free/
 * calloc/realloc/setmalloctag family (all built on the sbrk()-backed
 * hunk arena via halloc(), see sub.c's own gethunk() comment) is
 * DISABLED via #if 0 so real libc malloc()/calloc()/free()/realloc()/
 * setmalloctag() (lib_core/libc/port/minimal_malloc.c) are linked in
 * instead. Keeping any one of these locally defined while the others
 * fall through to libc.a would either recurse (malloc->halloc->
 * gethunk->malloc) or duplicate-symbol-collide (minimal_malloc.c
 * defines all of these in one object file).
 */
#if 0
/*
 * fake malloc
 */
void*
malloc(usize n)
{
	return halloc(n);
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(usize m, usize n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

/*
 * not used by compiler or loader, but Windows needs it
 */
void*
realloc(void *p, usize n)
{
	void *new;

	new = malloc(n);
	if(new != nil && p != nil)
		memmove(new, p, n);	/* safe only when adjecent hunks have no gaps */
	return new;
}

void
setmalloctag(void *v, uint32 pc)
{
	USED(v);
	USED(pc);
}
#endif
