#include <u.h>
#include <libc.h>

/* TEMPORARY placeholder allocator, deliberately named minimal_malloc.c
 * (not malloc.c) so it reads as a stand-in, not the real thing: a bump
 * allocator, enough for a benchmark that mallocs and never frees (see
 * benchs/compcert/lists.c, the first consumer), but NOT a real
 * allocator -- free() is a no-op, so any program that actually reuses
 * freed memory (most of them, once they need real allocation churn --
 * see benchs/compcert's binarytrees.c, which calls free() expecting
 * the space back) will eventually exhaust heap[] and abort(), not
 * silently misbehave.
 *
 * The plan (per the user, not yet done) is to replace this with a
 * real allocator ported from ~/principia/lib_core/libc/port/pool.c
 * (Plan9's real pool allocator), the same "adapt from principia,
 * re-verify empirically" pattern already used for several other
 * lib_core/libc/port/*.c files -- see
 * docs/claude_notes/notes_libc_selfhost.txt. When that lands, this
 * file should be deleted outright (not kept as a fallback), and every
 * lib_core/libc/mkfile PORTOFILES reference updated to point at
 * pool.c instead.
 */

/* 64MB, not a small round number chosen carefully -- since free() is
 * a no-op, this is really "total lifetime allocation across the whole
 * process", not "working set", and it doesn't take much to blow past
 * a small ceiling: benchs/compcert/nsieve.c alone mallocs ~9MB across
 * three never-actually-reclaimed calls. Bump further if a future
 * benchmark needs more, rather than treating this number as
 * meaningful -- it isn't, it's just "big enough for what's been tried
 * so far". Uninitialized, so this lands in BSS (demand-paged zero
 * pages), not 64MB of real file/memory committed upfront. */
enum { HEAPSIZE = 64*1024*1024 };

static char heap[HEAPSIZE];
static char *heapnext = heap;

void*
malloc(ulong n)
{
	char *p;

	/* 8-byte align, so double/vlong fields inside the allocation
	 * are naturally aligned regardless of what the caller stores
	 * there first. */
	n = (n + 7) & ~7UL;
	if(heapnext + n > heap + HEAPSIZE)
		abort();
	p = heapnext;
	heapnext += n;
	return p;
}

void*
calloc(ulong n, ulong size)
{
	void *p;

	p = malloc(n * size);
	memset(p, 0, n * size);
	return p;
}

void
free(void *p)
{
	USED(p);
}
