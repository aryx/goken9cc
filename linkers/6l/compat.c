#include	"l.h"

/*
 * claude: same fix as 8l/compat.c and 7l/falloc.c+sub.c (docs/
 * claude_notes/notes_os_macos.txt's "gethunk()/sbrk() OOM bug" section,
 * hit here too while runtime-testing the mk/rc self-host build on real
 * macOS/arm64 hardware -- Rosetta-translated darwin/amd64 6l, same
 * story) -- this whole fake malloc/free/calloc/realloc/halloc/
 * setmalloctag family (built on mysbrk()'s sbrk(), which macOS caps at
 * ~4MB and then returns -1) is DISABLED via #if 0, exactly matching
 * the TODO this file already carried below: real libc malloc()/
 * calloc()/free()/realloc()/setmalloctag() (lib_core/libc/port/
 * minimal_malloc.c) link in instead. linkers/lk/macho.c (shared with
 * 7l) no longer calls halloc() at all -- fixed directly to call
 * malloc()/mallocz(), same round -- so halloc() itself needs no
 * replacement here.
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
 * Nothing here actually needs a non-freeing arena today -- done above:
 *   - free() below is a no-op today; a real free() will actually
 *     reclaim memory, so any code that keeps a pointer around after
 *     "freeing" it (relying on the arena's never-really-freed
 *     semantics) will start reading/writing freed memory -- audit
 *     every free() callsite in these linkers first.
 *   - the hidden size header and its pointer arithmetic (here and in
 *     realloc()) are only needed by this arena; dropped below --
 *     real malloc/realloc already track block sizes themselves.
 *   - hunk/nhunk/gethunk()/mysbrk() (obj.c) are now dead code (every
 *     former direct caller switched to malloc()/mallocz(), same as
 *     7l's own gethunk() removal) but left in place, same as 8l's own
 *     frozen `thunk` global -- only ever read by a debug['v']
 *     diagnostic print, not correctness-relevant.
 *
 * claude: was `#if 0`/`#endif` -- goken's own compilers have NO `#if`
 * support at all (CLAUDE.md's own note: "#if defined(X) || defined(Y)
 * fails with 'unknown #: if'", and a bare `#if 0` fails identically,
 * confirmed fixing the identical mistake in linkers/7l/falloc.c --
 * this file hadn't been self-hosted yet to catch it here too, fixed
 * proactively while addressing the same pattern there). Switched to
 * individual `//` line comments instead, matching 8l/compat.c's own
 * established convention for this exact situation.
 */
/*
 * fake malloc
 *
 * claude: every block is prefixed with a hidden vlong holding its
 * requested size, so realloc() below can grow a block (needed by
 * lk/macho.c's machorebase(), which reallocs its offset array). The
 * header keeps the returned pointer 8-byte aligned since it is itself
 * 8 bytes and hunk only ever advances by multiples of 8.
 */
//void*
//malloc(ulong n)
//{
//	vlong *p;
//	ulong n2;
//
//	n2 = n + sizeof(vlong);
//	while(n2 & 7)
//		n2++;
//	while(nhunk < n2)
//		gethunk();
//	p = (vlong*)hunk;
//	*p = n;
//	nhunk -= n2;
//	hunk += n2;
//	return p+1;
//}
//
//void
//free(void *p)
//{
//	USED(p);
//}
//
//void*
//calloc(ulong m, ulong n)
//{
//	void *p;
//
//	n *= m;
//	p = malloc(n);
//	memset(p, 0, n);
//	return p;
//}
//
//void*
//realloc(void *v, ulong n)
//{
//	void *p;
//	vlong oldn;
//
//	if(v == nil)
//		return malloc(n);
//	oldn = ((vlong*)v)[-1];
//	if(n <= oldn)
//		return v;
//	p = malloc(n);
//	memmove(p, v, oldn);
//	return p;
//}
//
//void*
//mysbrk(ulong size)
//{
//	return sbrk(size);
//}
//
///* claude: lk/macho.c (shared with 7l) allocates its MachoLoad/MachoSect
// * arrays via halloc(); 7l has its own hunk-based halloc() in sub.c, but
// * that's functionally identical to our malloc() above, so just reuse it */
//void*
//halloc(ulong n)
//{
//	return malloc(n);
//}
//
//void
//setmalloctag(void*, ulong)
//{
//}

//old: now in libc
//int
//fileexists(char *s)
//{
//	uchar dirbuf[400];
//
//	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
//	return stat(s, dirbuf, sizeof(dirbuf)) >= 0;
//}
