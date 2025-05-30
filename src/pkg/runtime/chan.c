// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"
#include "type.h"

static	int32	debug	= 0;

enum
{
	Wclosed		= 0x0001,	// writer has closed
	Rclosed		= 0x0002,	// reader has seen close
	Eincr		= 0x0004,	// increment errors
	Emax		= 0x0800,	// error limit before throw
};

typedef	struct	Link	Link;
typedef	struct	WaitQ	WaitQ;
typedef	struct	SudoG	SudoG;
typedef	struct	Select	Select;
typedef	struct	Scase	Scase;

struct	SudoG
{
	G*	g;		// g and selgen constitute
	uint32	selgen;		// a weak pointer to g
	int16	offset;		// offset of case number
	int8	isfree;		// offset of case number
	SudoG*	link;
	byte	elem[8];	// synch data element (+ more)
};

struct	WaitQ
{
	SudoG*	first;
	SudoG*	last;
};

struct	Hchan
{
	uint32	qcount;			// total data in the q
	uint32	dataqsiz;		// size of the circular q
	uint16	elemsize;
	uint16	closed;			// Wclosed Rclosed errorcount
	uint8	elemalign;
	Alg*	elemalg;		// interface for element type
	Link*	senddataq;		// pointer for sender
	Link*	recvdataq;		// pointer for receiver
	WaitQ	recvq;			// list of recv waiters
	WaitQ	sendq;			// list of send waiters
	SudoG*	free;			// freelist
	Lock;
};

struct	Link
{
	Link*	link;			// asynch queue circular linked list
	byte	elem[8];		// asynch queue data element (+ more)
};

struct	Scase
{
	Hchan*	chan;			// chan
	byte*	pc;			// return pc
	uint16	send;			// 0-recv 1-send 2-default
	uint16	so;			// vararg of selected bool
	union {
		byte	elem[8];	// element (send)
		byte*	elemp;		// pointer to element (recv)
	} u;
};

struct	Select
{
	uint16	tcase;			// total count of scase[]
	uint16	ncase;			// currently filled scase[]
	Select*	link;			// for freelist
	Scase*	scase[1];		// one per case
};

static	void	dequeueg(WaitQ*, Hchan*);
static	SudoG*	dequeue(WaitQ*, Hchan*);
static	void	enqueue(WaitQ*, SudoG*);
static	SudoG*	allocsg(Hchan*);
static	void	freesg(Hchan*, SudoG*);
static	uint32	gcd(uint32, uint32);
static	uint32	fastrand1(void);
static	uint32	fastrand2(void);
static	void	destroychan(Hchan*);

Hchan*
makechan(Type *elem, int64 hint)
{
	Hchan *c;
	int32 i;

	if(hint < 0 || (int32)hint != hint || hint > ((uintptr)-1) / elem->size)
		panicstring("makechan: size out of range");

	if(elem->alg >= nelem(algarray)) {
		printf("chan(alg=%d)\n", elem->alg);
		throw("runtime.makechan: unsupported elem type");
	}

	c = mal(sizeof(*c));
	addfinalizer(c, destroychan, 0);

	c->elemsize = elem->size;
	c->elemalg = &algarray[elem->alg];
	c->elemalign = elem->align;

	if(hint > 0) {
		Link *d, *b, *e;

		// make a circular q
		b = nil;
		e = nil;
		for(i=0; i<hint; i++) {
			d = mal(sizeof(*d) + c->elemsize - sizeof(d->elem));
			if(e == nil)
				e = d;
			d->link = b;
			b = d;
		}
		e->link = b;
		c->recvdataq = b;
		c->senddataq = b;
		c->qcount = 0;
		c->dataqsiz = hint;
	}

	if(debug)
		printf("makechan: chan=%p; elemsize=%D; elemalg=%d; elemalign=%d; dataqsiz=%d\n",
			c, (int64)elem->size, elem->alg, elem->align, c->dataqsiz);

	return c;
}

static void
destroychan(Hchan *c)
{
	destroylock(&c->Lock);
}


// makechan(elem *Type, hint int64) (hchan *chan any);
void
·makechan(Type *elem, int64 hint, Hchan *ret)
{
	ret = makechan(elem, hint);
	FLUSH(&ret);
}

static void
incerr(Hchan* c)
{
	c->closed += Eincr;
	if(c->closed & Emax) {
		// Note that channel locks may still be held at this point.
		throw("too many operations on a closed channel");
	}
}

/*
 * generic single channel send/recv
 * if the bool pointer is nil,
 * then the full exchange will
 * occur. if pres is not nil,
 * then the protocol will not
 * sleep but return if it could
 * not complete.
 *
 * sleep can wake up with g->param == nil
 * when a channel involved in the sleep has
 * been closed.  it is easiest to loop and re-run
 * the operation; we'll see that it's now closed.
 */
void
chansend(Hchan *c, byte *ep, bool *pres)
{
	SudoG *sg;
	G* gp;

	if(c == nil)
		panicstring("send to nil channel");

	if(gcwaiting)
		gosched();

	if(debug) {
		printf("chansend: chan=%p; elem=", c);
		c->elemalg->print(c->elemsize, ep);
		prints("\n");
	}

	lock(c);
loop:
	if(c->closed & Wclosed)
		goto closed;

	if(c->dataqsiz > 0)
		goto asynch;

	sg = dequeue(&c->recvq, c);
	if(sg != nil) {
		if(ep != nil)
			c->elemalg->copy(c->elemsize, sg->elem, ep);

		gp = sg->g;
		gp->param = sg;
		unlock(c);
		ready(gp);

		if(pres != nil)
			*pres = true;
		return;
	}

	if(pres != nil) {
		unlock(c);
		*pres = false;
		return;
	}

	sg = allocsg(c);
	if(ep != nil)
		c->elemalg->copy(c->elemsize, sg->elem, ep);
	g->param = nil;
	g->status = Gwaiting;
	enqueue(&c->sendq, sg);
	unlock(c);
	gosched();

	lock(c);
	sg = g->param;
	if(sg == nil)
		goto loop;
	freesg(c, sg);
	unlock(c);
	return;

asynch:
	if(c->closed & Wclosed)
		goto closed;

	if(c->qcount >= c->dataqsiz) {
		if(pres != nil) {
			unlock(c);
			*pres = false;
			return;
		}
		sg = allocsg(c);
		g->status = Gwaiting;
		enqueue(&c->sendq, sg);
		unlock(c);
		gosched();

		lock(c);
		goto asynch;
	}
	if(ep != nil)
		c->elemalg->copy(c->elemsize, c->senddataq->elem, ep);
	c->senddataq = c->senddataq->link;
	c->qcount++;

	sg = dequeue(&c->recvq, c);
	if(sg != nil) {
		gp = sg->g;
		freesg(c, sg);
		unlock(c);
		ready(gp);
	} else
		unlock(c);
	if(pres != nil)
		*pres = true;
	return;

closed:
	incerr(c);
	if(pres != nil)
		*pres = true;
	unlock(c);
}

void
chanrecv(Hchan* c, byte *ep, bool* pres)
{
	SudoG *sg;
	G *gp;

	if(c == nil)
		panicstring("receive from nil channel");

	if(gcwaiting)
		gosched();

	if(debug)
		printf("chanrecv: chan=%p\n", c);

	lock(c);
loop:
	if(c->dataqsiz > 0)
		goto asynch;

	if(c->closed & Wclosed)
		goto closed;

	sg = dequeue(&c->sendq, c);
	if(sg != nil) {
		c->elemalg->copy(c->elemsize, ep, sg->elem);
		c->elemalg->copy(c->elemsize, sg->elem, nil);

		gp = sg->g;
		gp->param = sg;
		unlock(c);
		ready(gp);

		if(pres != nil)
			*pres = true;
		return;
	}

	if(pres != nil) {
		unlock(c);
		c->elemalg->copy(c->elemsize, ep, nil);
		*pres = false;
		return;
	}

	sg = allocsg(c);
	g->param = nil;
	g->status = Gwaiting;
	enqueue(&c->recvq, sg);
	unlock(c);
	gosched();

	lock(c);
	sg = g->param;
	if(sg == nil)
		goto loop;

	c->elemalg->copy(c->elemsize, ep, sg->elem);
	c->elemalg->copy(c->elemsize, sg->elem, nil);
	freesg(c, sg);
	unlock(c);
	return;

asynch:
	if(c->qcount <= 0) {
		if(c->closed & Wclosed)
			goto closed;

		if(pres != nil) {
			unlock(c);
			c->elemalg->copy(c->elemsize, ep, nil);
			*pres = false;
			return;
		}
		sg = allocsg(c);
		g->status = Gwaiting;
		enqueue(&c->recvq, sg);
		unlock(c);
		gosched();

		lock(c);
		goto asynch;
	}
	c->elemalg->copy(c->elemsize, ep, c->recvdataq->elem);
	c->elemalg->copy(c->elemsize, c->recvdataq->elem, nil);
	c->recvdataq = c->recvdataq->link;
	c->qcount--;
	sg = dequeue(&c->sendq, c);
	if(sg != nil) {
		gp = sg->g;
		freesg(c, sg);
		unlock(c);
		ready(gp);
		if(pres != nil)
			*pres = true;
		return;
	}

	unlock(c);
	if(pres != nil)
		*pres = true;
	return;

closed:
	c->elemalg->copy(c->elemsize, ep, nil);
	c->closed |= Rclosed;
	incerr(c);
	if(pres != nil)
		*pres = true;
	unlock(c);
}

// chansend1(hchan *chan any, elem any);
#pragma textflag 7
void
·chansend1(Hchan* c, ...)
{
	int32 o;
	byte *ae;

	if(c == nil)
		panicstring("send to nil channel");

	o = rnd(sizeof(c), c->elemalign);
	ae = (byte*)&c + o;
	chansend(c, ae, nil);
}

// chansend2(hchan *chan any, elem any) (pres bool);
#pragma textflag 7
void
·chansend2(Hchan* c, ...)
{
	int32 o;
	byte *ae, *ap;

	if(c == nil)
		panicstring("send to nil channel");

	o = rnd(sizeof(c), c->elemalign);
	ae = (byte*)&c + o;
	o = rnd(o+c->elemsize, Structrnd);
	ap = (byte*)&c + o;

	chansend(c, ae, ap);
}

// chanrecv1(hchan *chan any) (elem any);
#pragma textflag 7
void
·chanrecv1(Hchan* c, ...)
{
	int32 o;
	byte *ae;

	o = rnd(sizeof(c), Structrnd);
	ae = (byte*)&c + o;

	chanrecv(c, ae, nil);
}

// chanrecv2(hchan *chan any) (elem any, pres bool);
#pragma textflag 7
void
·chanrecv2(Hchan* c, ...)
{
	int32 o;
	byte *ae, *ap;

	o = rnd(sizeof(c), Structrnd);
	ae = (byte*)&c + o;
	o = rnd(o+c->elemsize, 1);
	ap = (byte*)&c + o;

	chanrecv(c, ae, ap);
}

// newselect(size uint32) (sel *byte);
#pragma textflag 7
void
·newselect(int32 size, ...)
{
	int32 n, o;
	Select **selp;
	Select *sel;

	o = rnd(sizeof(size), Structrnd);
	selp = (Select**)((byte*)&size + o);
	n = 0;
	if(size > 1)
		n = size-1;

	sel = mal(sizeof(*sel) + n*sizeof(sel->scase[0]));

	sel->tcase = size;
	sel->ncase = 0;
	*selp = sel;
	if(debug)
		printf("newselect s=%p size=%d\n", sel, size);
}

// selectsend(sel *byte, hchan *chan any, elem any) (selected bool);
#pragma textflag 7
void
·selectsend(Select *sel, Hchan *c, ...)
{
	int32 i, eo;
	Scase *cas;
	byte *ae;

	// nil cases do not compete
	if(c == nil)
		return;

	i = sel->ncase;
	if(i >= sel->tcase)
		throw("selectsend: too many cases");
	sel->ncase = i+1;
	cas = mal(sizeof *cas + c->elemsize - sizeof(cas->u.elem));
	sel->scase[i] = cas;

	cas->pc = ·getcallerpc(&sel);
	cas->chan = c;

	eo = rnd(sizeof(sel), sizeof(c));
	eo = rnd(eo+sizeof(c), c->elemsize);
	cas->so = rnd(eo+c->elemsize, Structrnd);
	cas->send = 1;

	ae = (byte*)&sel + eo;
	c->elemalg->copy(c->elemsize, cas->u.elem, ae);

	if(debug)
		printf("selectsend s=%p pc=%p chan=%p so=%d send=%d\n",
			sel, cas->pc, cas->chan, cas->so, cas->send);
}

// selectrecv(sel *byte, hchan *chan any, elem *any) (selected bool);
#pragma textflag 7
void
·selectrecv(Select *sel, Hchan *c, ...)
{
	int32 i, eo;
	Scase *cas;

	// nil cases do not compete
	if(c == nil)
		return;

	i = sel->ncase;
	if(i >= sel->tcase)
		throw("selectrecv: too many cases");
	sel->ncase = i+1;
	cas = mal(sizeof *cas);
	sel->scase[i] = cas;
	cas->pc = ·getcallerpc(&sel);
	cas->chan = c;

	eo = rnd(sizeof(sel), sizeof(c));
	eo = rnd(eo+sizeof(c), sizeof(byte*));
	cas->so = rnd(eo+sizeof(byte*), Structrnd);
	cas->send = 0;
	cas->u.elemp = *(byte**)((byte*)&sel + eo);

	if(debug)
		printf("selectrecv s=%p pc=%p chan=%p so=%d send=%d\n",
			sel, cas->pc, cas->chan, cas->so, cas->send);
}


// selectdefaul(sel *byte) (selected bool);
#pragma textflag 7
void
·selectdefault(Select *sel, ...)
{
	int32 i;
	Scase *cas;

	i = sel->ncase;
	if(i >= sel->tcase)
		throw("selectdefault: too many cases");
	sel->ncase = i+1;
	cas = mal(sizeof *cas);
	sel->scase[i] = cas;
	cas->pc = ·getcallerpc(&sel);
	cas->chan = nil;

	cas->so = rnd(sizeof(sel), Structrnd);
	cas->send = 2;
	cas->u.elemp = nil;

	if(debug)
		printf("selectdefault s=%p pc=%p so=%d send=%d\n",
			sel, cas->pc, cas->so, cas->send);
}

static void
freesel(Select *sel)
{
	uint32 i;

	for(i=0; i<sel->ncase; i++)
		free(sel->scase[i]);
	free(sel);
}

static void
sellock(Select *sel)
{
	uint32 i;
	Hchan *c;

	c = nil;
	for(i=0; i<sel->ncase; i++) {
		if(sel->scase[i]->chan != c) {
			c = sel->scase[i]->chan;
			lock(c);
		}
	}
}

static void
selunlock(Select *sel)
{
	uint32 i;
	Hchan *c;

	c = nil;
	for(i=sel->ncase; i>0; i--) {
		if(sel->scase[i-1]->chan && sel->scase[i-1]->chan != c) {
			c = sel->scase[i-1]->chan;
			unlock(c);
		}
	}
}

// selectgo(sel *byte);
void
·selectgo(Select *sel)
{
	uint32 p, o, i, j;
	Scase *cas, *dfl;
	Hchan *c;
	SudoG *sg;
	G *gp;
	byte *as;

	if(gcwaiting)
		gosched();

	if(debug)
		printf("select: sel=%p\n", sel);

	if(sel->ncase < 2) {
		if(sel->ncase < 1) {
			g->status = Gwaiting;	// forever
			gosched();
		}
		// TODO: make special case of one.
	}

	// select a (relative) prime
	for(i=0;; i++) {
		p = fastrand1();
		if(gcd(p, sel->ncase) == 1)
			break;
		if(i > 1000)
			throw("select: failed to select prime");
	}

	// select an initial offset
	o = fastrand2();

	p %= sel->ncase;
	o %= sel->ncase;

	// sort the cases by Hchan address to get the locking order.
	for(i=1; i<sel->ncase; i++) {
		cas = sel->scase[i];
		for(j=i; j>0 && sel->scase[j-1]->chan >= cas->chan; j--)
			sel->scase[j] = sel->scase[j-1];
		sel->scase[j] = cas;
	}

	sellock(sel);

loop:
	// pass 1 - look for something already waiting
	dfl = nil;
	for(i=0; i<sel->ncase; i++) {
		cas = sel->scase[o];
		c = cas->chan;

		switch(cas->send) {
		case 0:	// recv
			if(c->dataqsiz > 0) {
				if(c->qcount > 0)
					goto asyncrecv;
			} else {
				sg = dequeue(&c->sendq, c);
				if(sg != nil)
					goto syncrecv;
			}
			if(c->closed & Wclosed)
				goto rclose;
			break;

		case 1:	// send
			if(c->closed & Wclosed)
				goto sclose;
			if(c->dataqsiz > 0) {
				if(c->qcount < c->dataqsiz)
					goto asyncsend;
			} else {
				sg = dequeue(&c->recvq, c);
				if(sg != nil)
					goto syncsend;
			}
			break;

		case 2:	// default
			dfl = cas;
			break;
		}

		o += p;
		if(o >= sel->ncase)
			o -= sel->ncase;
	}

	if(dfl != nil) {
		cas = dfl;
		goto retc;
	}


	// pass 2 - enqueue on all chans
	for(i=0; i<sel->ncase; i++) {
		cas = sel->scase[o];
		c = cas->chan;
		sg = allocsg(c);
		sg->offset = o;

		switch(cas->send) {
		case 0:	// recv
			if(c->dataqsiz > 0) {
				if(c->qcount > 0)
					throw("select: pass 2 async recv");
			} else {
				if(dequeue(&c->sendq, c))
					throw("select: pass 2 sync recv");
			}
			enqueue(&c->recvq, sg);
			break;
		
		case 1:	// send
			if(c->dataqsiz > 0) {
				if(c->qcount < c->dataqsiz)
					throw("select: pass 2 async send");
			} else {
				if(dequeue(&c->recvq, c))
					throw("select: pass 2 sync send");
				c->elemalg->copy(c->elemsize, sg->elem, cas->u.elem);
			}
			enqueue(&c->sendq, sg);
			break;
		}

		o += p;
		if(o >= sel->ncase)
			o -= sel->ncase;
	}

	g->param = nil;
	g->status = Gwaiting;
	selunlock(sel);
	gosched();

	sellock(sel);
	sg = g->param;

	// pass 3 - dequeue from unsuccessful chans
	// otherwise they stack up on quiet channels
	for(i=0; i<sel->ncase; i++) {
		if(sg == nil || o != sg->offset) {
			cas = sel->scase[o];
			c = cas->chan;
			if(cas->send)
				dequeueg(&c->sendq, c);
			else
				dequeueg(&c->recvq, c);
		}
		
		o += p;
		if(o >= sel->ncase)
			o -= sel->ncase;
	}

	if(sg == nil)
		goto loop;

	o = sg->offset;
	cas = sel->scase[o];
	c = cas->chan;

	if(c->dataqsiz > 0) {
//		prints("shouldnt happen\n");
		goto loop;
	}

	if(debug)
		printf("wait-return: sel=%p c=%p cas=%p send=%d o=%d\n",
			sel, c, cas, cas->send, o);

	if(!cas->send) {
		if(cas->u.elemp != nil)
			c->elemalg->copy(c->elemsize, cas->u.elemp, sg->elem);
		c->elemalg->copy(c->elemsize, sg->elem, nil);
	}

	freesg(c, sg);
	goto retc;

asyncrecv:
	// can receive from buffer
	if(cas->u.elemp != nil)
		c->elemalg->copy(c->elemsize, cas->u.elemp, c->recvdataq->elem);
	c->elemalg->copy(c->elemsize, c->recvdataq->elem, nil);
	c->recvdataq = c->recvdataq->link;
	c->qcount--;
	sg = dequeue(&c->sendq, c);
	if(sg != nil) {
		gp = sg->g;
		freesg(c, sg);
		ready(gp);
	}
	goto retc;

asyncsend:
	// can send to buffer
	if(cas->u.elem != nil)
		c->elemalg->copy(c->elemsize, c->senddataq->elem, cas->u.elem);
	c->senddataq = c->senddataq->link;
	c->qcount++;
	sg = dequeue(&c->recvq, c);
	if(sg != nil) {
		gp = sg->g;
		freesg(c, sg);
		ready(gp);
	}
	goto retc;

syncrecv:
	// can receive from sleeping sender (sg)
	if(debug)
		printf("syncrecv: sel=%p c=%p o=%d\n", sel, c, o);
	if(cas->u.elemp != nil)
		c->elemalg->copy(c->elemsize, cas->u.elemp, sg->elem);
	c->elemalg->copy(c->elemsize, sg->elem, nil);
	gp = sg->g;
	gp->param = sg;
	ready(gp);
	goto retc;

rclose:
	// read at end of closed channel
	if(cas->u.elemp != nil)
		c->elemalg->copy(c->elemsize, cas->u.elemp, nil);
	c->closed |= Rclosed;
	incerr(c);
	goto retc;

syncsend:
	// can send to sleeping receiver (sg)
	if(debug)
		printf("syncsend: sel=%p c=%p o=%d\n", sel, c, o);
	if(c->closed & Wclosed)
		goto sclose;
	c->elemalg->copy(c->elemsize, sg->elem, cas->u.elem);
	gp = sg->g;
	gp->param = sg;
	ready(gp);
	goto retc;

sclose:
	// send on closed channel
	incerr(c);
	goto retc;

retc:
	selunlock(sel);

	// return to pc corresponding to chosen case
	·setcallerpc(&sel, cas->pc);
	as = (byte*)&sel + cas->so;
	freesel(sel);
	*as = true;
}

// closechan(sel *byte);
void
·closechan(Hchan *c)
{
	SudoG *sg;
	G* gp;

	if(gcwaiting)
		gosched();

	lock(c);
	incerr(c);
	c->closed |= Wclosed;

	// release all readers
	for(;;) {
		sg = dequeue(&c->recvq, c);
		if(sg == nil)
			break;
		gp = sg->g;
		gp->param = nil;
		freesg(c, sg);
		ready(gp);
	}

	// release all writers
	for(;;) {
		sg = dequeue(&c->sendq, c);
		if(sg == nil)
			break;
		gp = sg->g;
		gp->param = nil;
		freesg(c, sg);
		ready(gp);
	}

	unlock(c);
}

void
chanclose(Hchan *c)
{
	·closechan(c);
}

bool
chanclosed(Hchan *c)
{
	return (c->closed & Rclosed) != 0;
}

int32
chanlen(Hchan *c)
{
	return c->qcount;
}

int32
chancap(Hchan *c)
{
	return c->dataqsiz;
}


// closedchan(sel *byte) bool;
void
·closedchan(Hchan *c, bool closed)
{
	closed = chanclosed(c);
	FLUSH(&closed);
}

static SudoG*
dequeue(WaitQ *q, Hchan *c)
{
	SudoG *sgp;

loop:
	sgp = q->first;
	if(sgp == nil)
		return nil;
	q->first = sgp->link;

	// if sgp is stale, ignore it
	if(!cas(&sgp->g->selgen, sgp->selgen, sgp->selgen + 1)) {
		//prints("INVALID PSEUDOG POINTER\n");
		freesg(c, sgp);
		goto loop;
	}

	return sgp;
}

static void
dequeueg(WaitQ *q, Hchan *c)
{
	SudoG **l, *sgp;
	
	for(l=&q->first; (sgp=*l) != nil; l=&sgp->link) {
		if(sgp->g == g) {
			*l = sgp->link;
			freesg(c, sgp);
			break;
		}
	}
}

static void
enqueue(WaitQ *q, SudoG *sgp)
{
	sgp->link = nil;
	if(q->first == nil) {
		q->first = sgp;
		q->last = sgp;
		return;
	}
	q->last->link = sgp;
	q->last = sgp;
}

static SudoG*
allocsg(Hchan *c)
{
	SudoG* sg;

	sg = c->free;
	if(sg != nil) {
		c->free = sg->link;
	} else
		sg = mal(sizeof(*sg) + c->elemsize - sizeof(sg->elem));
	sg->selgen = g->selgen;
	sg->g = g;
	sg->offset = 0;
	sg->isfree = 0;

	return sg;
}

static void
freesg(Hchan *c, SudoG *sg)
{
	if(sg != nil) {
		if(sg->isfree)
			throw("chan.freesg: already free");
		sg->isfree = 1;
		sg->link = c->free;
		c->free = sg;
	}
}

static uint32
gcd(uint32 u, uint32 v)
{
	for(;;) {
		if(u > v) {
			if(v == 0)
				return u;
			u = u%v;
			continue;
		}
		if(u == 0)
			return v;
		v = v%u;
	}
}

static uint32
fastrand1(void)
{
	static uint32 x = 0x49f6428aUL;

	x += x;
	if(x & 0x80000000L)
		x ^= 0x88888eefUL;
	return x;
}

static uint32
fastrand2(void)
{
	static uint32 x = 0x49f6428aUL;

	x += x;
	if(x & 0x80000000L)
		x ^= 0xfafd871bUL;
	return x;
}
