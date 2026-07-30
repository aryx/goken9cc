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

/* ulong, matching include/core/mem.h's own extern malloc(ulong)/
 * free(void*) declarations (already present before this file existed
 * -- this just fills in the implementation) and calloc(ulong, ulong)
 * (was commented out there, uncommented as part of adding this file).
 * Yes, this means allocations are capped at 4GB in principle (`long`
 * is only 4 bytes on this compiler even on 64-bit arches -- see
 * docs/claude_notes/notes_libc_selfhost.txt) -- an existing, already-
 * decided project convention this file matches rather than overrides;
 * nothing built against this libc so far needs more than a few MB.
 */
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

/* claude: same "temporary, not the real thing" spirit as malloc/calloc
 * above -- this bump allocator never tracks each allocation's real
 * size, so a real realloc() (which must copy only min(oldsize,
 * newsize) bytes) isn't possible; this always copies `n` (the *new*
 * size) bytes from the old block instead. Reading `n` bytes from a
 * smaller old allocation reads past whatever that specific block
 * "owned" -- harmless in practice for any p that's still comfortably
 * inside heap[] (true for every allocation this bump allocator has
 * ever handed out, short of one sitting within `n` bytes of
 * heap+HEAPSIZE, which nothing here does), but NOT a guaranteed-safe
 * operation the way malloc/free/calloc above are. The grown tail's
 * *contents* are unspecified either way (not zeroed), same as real
 * POSIX realloc()'s own contract. Fine for
 * benchs/compcert/knucleotide.c (the first, and so far only, caller):
 * it always overwrites the grown tail itself before reading it.
 */
void*
realloc(void *p, ulong n)
{
	void *q;

	if(p == nil)
		return malloc(n);
	q = malloc(n);
	memcpy(q, p, n);
	return q;
}

/* claude: no-op debug-tag bookkeeping (include/core/mem.h's own
 * declarations, previously unimplemented -- callable but never called
 * before port/strdup.c, adapted from ~/principia-softwarica/lib_core/
 * libc/port/strdup.c, started calling setmalloctag()). A real
 * allocator would use these to record "who allocated this block" (the
 * caller's PC, from getcallerpc()) for leak-tracking/debugging; this
 * bump allocator tracks nothing about individual allocations at all
 * (not even their size, see realloc() above), so there's nowhere to
 * actually store a tag -- these are safe no-ops, not a real
 * implementation deferred for later the way getcallerpc() itself is
 * (lib_core/libc/arch/{386,amd64,arm}/getcallerpc.s, port/
 * getcallerpc.c): nothing here would change even once malloc() tracks
 * per-block metadata, unless that metadata includes tags too.
 */
void
setmalloctag(void *v, ulong pc)
{
	USED(v);
	USED(pc);
}

ulong
getmalloctag(void *v)
{
	USED(v);
	return 0;
}

ulong
getrealloctag(void *v)
{
	USED(v);
	return 0;
}
