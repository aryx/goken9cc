// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Garbage collector -- step 0.
//
// Stop the world, mark and sweep garbage collector.
// NOT INTENDED FOR PRODUCTION USE.
//
// A mark and sweep collector provides a way to exercise
// and test the memory allocator and the stack walking machinery
// without also needing to get reference counting
// exactly right.

#include "runtime.h"
#include "malloc.h"

enum {
	Debug = 0
};

typedef struct BlockList BlockList;
struct BlockList
{
	byte *obj;
	uintptr size;
};

extern byte data[];
extern byte etext[];
extern byte end[];

static G *fing;
static Finalizer *finq;
static int32 fingwait;
static BlockList *bl, *ebl;

static void runfinq(void);

enum {
	PtrSize = sizeof(void*)
};

static void
scanblock(byte *b, int64 n)
{
	int32 off;
	void *obj;
	uintptr size;
	uint32 *refp, ref;
	void **vp;
	int64 i;
	BlockList *w;

	w = bl;
	w->obj = b;
	w->size = n;
	w++;

	while(w > bl) {
		w--;
		b = w->obj;
		n = w->size;

		if(Debug > 1)
			printf("scanblock %p %D\n", b, n);
		off = (uint32)(uintptr)b & (PtrSize-1);
		if(off) {
			b += PtrSize - off;
			n -= PtrSize - off;
		}
	
		vp = (void**)b;
		n /= PtrSize;
		for(i=0; i<n; i++) {
			obj = vp[i];
			if(obj == nil)
				continue;
			if(mheap.closure_min != nil && mheap.closure_min <= (byte*)obj && (byte*)obj < mheap.closure_max) {
				if((((uintptr)obj) & 63) != 0)
					continue;
	
				// Looks like a Native Client closure.
				// Actual pointer is pointed at by address in first instruction.
				// Embedded pointer starts at byte 2.
				// If it is f4f4f4f4 then that space hasn't been
				// used for a closure yet (f4 is the HLT instruction).
				// See nacl/386/closure.c for more.
				void **pp;
				pp = *(void***)((byte*)obj+2);
				if(pp == (void**)0xf4f4f4f4)	// HLT... - not a closure after all
					continue;
				obj = *pp;
			}
			if(mheap.min <= (byte*)obj && (byte*)obj < mheap.max) {
				if(mlookup(obj, &obj, &size, nil, &refp)) {
					ref = *refp;
					switch(ref & ~RefFlags) {
					case RefNone:
						if(Debug > 1)
							printf("found at %p: ", &vp[i]);
						*refp = RefSome | (ref & RefFlags);
						if(!(ref & RefNoPointers)) {
							if(w >= ebl)
								throw("scanblock: garbage collection stack overflow");
							w->obj = obj;
							w->size = size;
							w++;
						}
						break;
					}
				}
			}
		}
	}
}

static void
scanstack(G *gp)
{
	Stktop *stk;
	byte *sp;

	if(gp == g)
		sp = (byte*)&gp;
	else
		sp = gp->sched.sp;
	if(Debug > 1)
		printf("scanstack %d %p\n", gp->goid, sp);
	stk = (Stktop*)gp->stackbase;
	while(stk) {
		scanblock(sp, (byte*)stk - sp);
		sp = stk->gobuf.sp;
		stk = (Stktop*)stk->stackbase;
	}
}

static void
markfin(void *v)
{
	uintptr size;
	uint32 *refp;

	size = 0;
	refp = nil;
	if(!mlookup(v, &v, &size, nil, &refp) || !(*refp & RefHasFinalizer))
		throw("mark - finalizer inconsistency");
	
	// do not mark the finalizer block itself.  just mark the things it points at.
	scanblock(v, size);
}

static void
mark(void)
{
	G *gp;
	uintptr blsize, nobj;

	// Figure out how big an object stack we need.
	// Get a new one if we need more than we have
	// or we need significantly less than we have.
	nobj = mstats.heap_objects;
	if(nobj > ebl - bl || nobj < (ebl-bl)/4) {
		if(bl != nil)
			SysFree(bl, (byte*)ebl - (byte*)bl);
		
		// While we're allocated a new object stack,
		// add 20% headroom and also round up to
		// the nearest page boundary, since mmap
		// will anyway.
		nobj = nobj * 12/10;
		blsize = nobj * sizeof *bl;
		blsize = (blsize + 4095) & ~4095;
		nobj = blsize / sizeof *bl;
		bl = SysAlloc(blsize);
		ebl = bl + nobj;
	}

	// mark data+bss.
	// skip mheap itself, which has no interesting pointers
	// and is mostly zeroed and would not otherwise be paged in.
	scanblock(data, (byte*)&mheap - data);
	scanblock((byte*)(&mheap+1), end - (byte*)(&mheap+1));

	// mark stacks
	for(gp=allg; gp!=nil; gp=gp->alllink) {
		switch(gp->status){
		default:
			printf("unexpected G.status %d\n", gp->status);
			throw("mark - bad status");
		case Gdead:
			break;
		case Grunning:
		case Grecovery:
			if(gp != g)
				throw("mark - world not stopped");
			scanstack(gp);
			break;
		case Grunnable:
		case Gsyscall:
		case Gwaiting:
			scanstack(gp);
			break;
		}
	}

	// mark things pointed at by objects with finalizers
	walkfintab(markfin);
}

// free RefNone, free & queue finalizers for RefNone|RefHasFinalizer, reset RefSome
static void
sweepspan(MSpan *s)
{
	int32 n, npages, size;
	byte *p;
	uint32 ref, *gcrefp, *gcrefep;
	MCache *c;
	Finalizer *f;

	p = (byte*)(s->start << PageShift);
	if(s->sizeclass == 0) {
		// Large block.
		ref = s->gcref0;
		switch(ref & ~(RefFlags^RefHasFinalizer)) {
		case RefNone:
			// Free large object.
			mstats.alloc -= s->npages<<PageShift;
			runtime_memclr(p, s->npages<<PageShift);
			if(ref & RefProfiled)
				MProf_Free(p, s->npages<<PageShift);
			s->gcref0 = RefFree;
			MHeap_Free(&mheap, s, 1);
			break;
		case RefNone|RefHasFinalizer:
			f = getfinalizer(p, 1);
			if(f == nil)
				throw("finalizer inconsistency");
			f->arg = p;
			f->next = finq;
			finq = f;
			ref &= ~RefHasFinalizer;
			// fall through
		case RefSome:
		case RefSome|RefHasFinalizer:
			s->gcref0 = RefNone | (ref&RefFlags);
			break;
		}
		return;
	}

	// Chunk full of small blocks.
	MGetSizeClassInfo(s->sizeclass, &size, &npages, &n);
	gcrefp = s->gcref;
	gcrefep = s->gcref + n;
	for(; gcrefp < gcrefep; gcrefp++, p += size) {
		ref = *gcrefp;
		if(ref < RefNone)	// RefFree or RefStack
			continue;
		switch(ref & ~(RefFlags^RefHasFinalizer)) {
		case RefNone:
			// Free small object.
			if(ref & RefProfiled)
				MProf_Free(p, size);
			*gcrefp = RefFree;
			c = m->mcache;
			if(size > sizeof(uintptr))
				((uintptr*)p)[1] = 1;	// mark as "needs to be zeroed"
			mstats.alloc -= size;
			mstats.by_size[s->sizeclass].nfree++;
			MCache_Free(c, p, s->sizeclass, size);
			break;
		case RefNone|RefHasFinalizer:
			f = getfinalizer(p, 1);
			if(f == nil)
				throw("finalizer inconsistency");
			f->arg = p;
			f->next = finq;
			finq = f;
			ref &= ~RefHasFinalizer;
			// fall through
		case RefSome:
		case RefSome|RefHasFinalizer:
			*gcrefp = RefNone | (ref&RefFlags);
			break;
		}
	}
}

static void
sweep(void)
{
	MSpan *s;

	for(s = mheap.allspans; s != nil; s = s->allnext)
		if(s->state == MSpanInUse)
			sweepspan(s);
}

// Semaphore, not Lock, so that the goroutine
// reschedules when there is contention rather
// than spinning.
static uint32 gcsema = 1;

// Initialized from $GOGC.  GOGC=off means no gc.
//
// Next gc is after we've allocated an extra amount of
// memory proportional to the amount already in use.
// If gcpercent=100 and we're using 4M, we'll gc again
// when we get to 8M.  This keeps the gc cost in linear
// proportion to the allocation cost.  Adjusting gcpercent
// just changes the linear constant (and also the amount of
// extra memory used).
static int32 gcpercent = -2;

static void
stealcache(void)
{
	M *m;
	
	for(m=allm; m; m=m->alllink)
		MCache_ReleaseAll(m->mcache);
}

static void
cachestats(void)
{
	M *m;
	MCache *c;

	for(m=allm; m; m=m->alllink) {
		c = m->mcache;
		mstats.heap_alloc += c->local_alloc;
		c->local_alloc = 0;
		mstats.heap_objects += c->local_objects;
		c->local_objects = 0;
	}
}

void
gc(int32 force)
{
	int64 t0, t1;
	byte *p;
	Finalizer *fp;

	// The gc is turned off (via enablegc) until
	// the bootstrap has completed.
	// Also, malloc gets called in the guts
	// of a number of libraries that might be
	// holding locks.  To avoid priority inversion
	// problems, don't bother trying to run gc
	// while holding a lock.  The next mallocgc
	// without a lock will do the gc instead.
	if(!mstats.enablegc || m->locks > 0 || panicking)
		return;

	if(gcpercent == -2) {	// first time through
		p = getenv("GOGC");
		if(p == nil || p[0] == '\0')
			gcpercent = 100;
		else if(strcmp(p, (byte*)"off") == 0)
			gcpercent = -1;
		else
			gcpercent = atoi(p);
	}
	if(gcpercent < 0)
		return;

	semacquire(&gcsema);
	t0 = nanotime();
	m->gcing = 1;
	stoptheworld();
	if(mheap.Lock.key != 0)
		throw("mheap locked during gc");
	if(force || mstats.heap_alloc >= mstats.next_gc) {
		cachestats();
		mark();
		sweep();
		stealcache();
		mstats.next_gc = mstats.heap_alloc+mstats.heap_alloc*gcpercent/100;
	}
	m->gcing = 0;

	m->locks++;	// disable gc during the mallocs in newproc
	fp = finq;
	if(fp != nil) {
		// kick off or wake up goroutine to run queued finalizers
		if(fing == nil)
			fing = newproc1((byte*)runfinq, nil, 0, 0);
		else if(fingwait) {
			fingwait = 0;
			ready(fing);
		}
	}
	m->locks--;

	t1 = nanotime();
	mstats.numgc++;
	mstats.pause_ns += t1 - t0;
	if(mstats.debuggc)
		printf("pause %D\n", t1-t0);
	semrelease(&gcsema);
	starttheworld();
	
	// give the queued finalizers, if any, a chance to run
	if(fp != nil)
		gosched();
}

static void
runfinq(void)
{
	Finalizer *f, *next;
	byte *frame;

	for(;;) {
		// There's no need for a lock in this section
		// because it only conflicts with the garbage
		// collector, and the garbage collector only
		// runs when everyone else is stopped, and
		// runfinq only stops at the gosched() or
		// during the calls in the for loop.
		f = finq;
		finq = nil;
		if(f == nil) {
			fingwait = 1;
			g->status = Gwaiting;
			gosched();
			continue;
		}
		for(; f; f=next) {
			next = f->next;
			frame = mal(sizeof(uintptr) + f->nret);
			*(void**)frame = f->arg;
			reflect·call((byte*)f->fn, frame, sizeof(uintptr) + f->nret);
			free(frame);
			f->fn = nil;
			f->arg = nil;
			f->next = nil;
			free(f);
		}
		gc(1);	// trigger another gc to clean up the finalized objects, if possible
	}
}
