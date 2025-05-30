// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "runtime.h"

int32	panicking	= 0;
int32	maxround	= sizeof(uintptr);
int32	fd		= 1;

int32
gotraceback(void)
{
	byte *p;

	p = getenv("GOTRACEBACK");
	if(p == nil || p[0] == '\0')
		return 1;	// default is on
	return atoi(p);
}

void
panic(int32 unused)
{
	fd = 2;
	if(panicking) {
		printf("double panic\n");
		exit(3);
	}
	panicking++;

	printf("\npanic PC=%X\n", (uint64)(uintptr)&unused);
	if(gotraceback()){
		traceback(·getcallerpc(&unused), getcallersp(&unused), 0, g);
		tracebackothers(g);
	}
	breakpoint();  // so we can grab it in a debugger
	exit(2);
}

void
·panicindex(void)
{
	panicstring("index out of range");
}

void
·panicslice(void)
{
	panicstring("slice bounds out of range");
}

void
·throwreturn(void)
{
	// can only happen if compiler is broken
	throw("no return at end of a typed function - compiler is broken");
}

void
·throwinit(void)
{
	// can only happen with linker skew
	throw("recursive call during initialization - linker skew");
}

void
throw(int8 *s)
{
	fd = 2;
	printf("throw: %s\n", s);
	panic(-1);
	*(int32*)0 = 0;	// not reached
	exit(1);	// even more not reached
}

void
panicstring(int8 *s)
{
	Eface err;
	
	·newErrorString(gostringnocopy((byte*)s), &err);
	·panic(err);
}

void
mcpy(byte *t, byte *f, uint32 n)
{
	while(n > 0) {
		*t = *f;
		t++;
		f++;
		n--;
	}
}

int32
mcmp(byte *s1, byte *s2, uint32 n)
{
	uint32 i;
	byte c1, c2;

	for(i=0; i<n; i++) {
		c1 = s1[i];
		c2 = s2[i];
		if(c1 < c2)
			return -1;
		if(c1 > c2)
			return +1;
	}
	return 0;
}


byte*
mchr(byte *p, byte c, byte *ep)
{
	for(; p < ep; p++)
		if(*p == c)
			return p;
	return nil;
}

uint32
rnd(uint32 n, uint32 m)
{
	uint32 r;

	if(m > maxround)
		m = maxround;
	r = n % m;
	if(r)
		n += m-r;
	return n;
}

static int32	argc;
static uint8**	argv;

Slice os·Args;
Slice os·Envs;

void
args(int32 c, uint8 **v)
{
	argc = c;
	argv = v;
}

int32 isplan9;

void
goargs(void)
{
	String *gargv;
	String *genvv;
	int32 i, envc;
	
	if(isplan9)
		envc=0;
	else
		for(envc=0; argv[argc+1+envc] != 0; envc++)
			;

	gargv = malloc(argc*sizeof gargv[0]);
	genvv = malloc(envc*sizeof genvv[0]);

	for(i=0; i<argc; i++)
		gargv[i] = gostringnocopy(argv[i]);
	os·Args.array = (byte*)gargv;
	os·Args.len = argc;
	os·Args.cap = argc;

	for(i=0; i<envc; i++)
		genvv[i] = gostringnocopy(argv[argc+1+i]);
	os·Envs.array = (byte*)genvv;
	os·Envs.len = envc;
	os·Envs.cap = envc;
}

// Atomic add and return new value.
uint32
xadd(uint32 volatile *val, int32 delta)
{
	uint32 oval, nval;

	for(;;){
		oval = *val;
		nval = oval + delta;
		if(cas(val, oval, nval))
			return nval;
	}
}

byte*
getenv(int8 *s)
{
	int32 i, j, len;
	byte *v, *bs;
	String* envv;
	int32 envc;

	bs = (byte*)s;
	len = findnull(bs);
	envv = (String*)os·Envs.array;
	envc = os·Envs.len;
	for(i=0; i<envc; i++){
		if(envv[i].len <= len)
			continue;
		v = envv[i].str;
		for(j=0; j<len; j++)
			if(bs[j] != v[j])
				goto nomatch;
		if(v[len] != '=')
			goto nomatch;
		return v+len+1;
	nomatch:;
	}
	return nil;
}

void
·getgoroot(String out)
{
	byte *p;

	p = getenv("GOROOT");
	out = gostringnocopy(p);
	FLUSH(&out);
}

int32
atoi(byte *p)
{
	int32 n;

	n = 0;
	while('0' <= *p && *p <= '9')
		n = n*10 + *p++ - '0';
	return n;
}

void
check(void)
{
	int8 a;
	uint8 b;
	int16 c;
	uint16 d;
	int32 e;
	uint32 f;
	int64 g;
	uint64 h;
	float32 i;
	float64 j;
	void* k;
	uint16* l;

	if(sizeof(a) != 1) throw("bad a");
	if(sizeof(b) != 1) throw("bad b");
	if(sizeof(c) != 2) throw("bad c");
	if(sizeof(d) != 2) throw("bad d");
	if(sizeof(e) != 4) throw("bad e");
	if(sizeof(f) != 4) throw("bad f");
	if(sizeof(g) != 8) throw("bad g");
	if(sizeof(h) != 8) throw("bad h");
	if(sizeof(i) != 4) throw("bad i");
	if(sizeof(j) != 8) throw("bad j");
	if(sizeof(k) != sizeof(uintptr)) throw("bad k");
	if(sizeof(l) != sizeof(uintptr)) throw("bad l");
//	prints(1"check ok\n");

	uint32 z;
	z = 1;
	if(!cas(&z, 1, 2))
		throw("cas1");
	if(z != 2)
		throw("cas2");

	z = 4;
	if(cas(&z, 5, 6))
		throw("cas3");
	if(z != 4)
		throw("cas4");

	initsig(0);
}

/*
 * map and chan helpers for
 * dealing with unknown types
 */
static uintptr
memhash(uint32 s, void *a)
{
	byte *b;
	uintptr hash;

	b = a;
	if(sizeof(hash) == 4)
		hash = 2860486313U;
	else
		hash = 33054211828000289ULL;
	while(s > 0) {
		if(sizeof(hash) == 4)
			hash = (hash ^ *b) * 3267000013UL;
		else
			hash = (hash ^ *b) * 23344194077549503ULL;
		b++;
		s--;
	}
	return hash;
}

static uint32
memequal(uint32 s, void *a, void *b)
{
	byte *ba, *bb, *aend;
	uint32 i;

	ba = a;
	bb = b;
	aend = ba+s;
	while(ba != aend) {
		if(*ba != *bb)
			return 0;
		ba++;
		bb++;
	}
	return 1;
}

static void
memprint(uint32 s, void *a)
{
	uint64 v;

	v = 0xbadb00b;
	switch(s) {
	case 1:
		v = *(uint8*)a;
		break;
	case 2:
		v = *(uint16*)a;
		break;
	case 4:
		v = *(uint32*)a;
		break;
	case 8:
		v = *(uint64*)a;
		break;
	}
	·printint(v);
}

static void
memcopy(uint32 s, void *a, void *b)
{
	byte *ba, *bb;
	uint32 i;

	ba = a;
	bb = b;
	if(bb == nil) {
		for(i=0; i<s; i++)
			ba[i] = 0;
		return;
	}
	for(i=0; i<s; i++)
		ba[i] = bb[i];
}

static uint32
memwordequal(uint32 s, void *a, void *b)
{
	USED(s);
	return *(uintptr*)(a) == *(uintptr*)(b);
}

static void
memwordcopy(uint32 s, void *a, void *b)
{
	USED(s);
	if (b == nil) {
		*(uintptr*)(a) = 0;
		return;
	}
	*(uintptr*)(a) = *(uintptr*)(b);
}

static uintptr
strhash(uint32 s, String *a)
{
	USED(s);
	return memhash((*a).len, (*a).str);
}

static uint32
strequal(uint32 s, String *a, String *b)
{
	int32 alen;

	USED(s);
	alen = a->len;
	if(alen != b->len)
		return false;
	return memequal(alen, a->str, b->str);
}

static void
strprint(uint32 s, String *a)
{
	USED(s);
	·printstring(*a);
}

static uintptr
interhash(uint32 s, Iface *a)
{
	USED(s);
	return ifacehash(*a);
}

static void
interprint(uint32 s, Iface *a)
{
	USED(s);
	·printiface(*a);
}

static uint32
interequal(uint32 s, Iface *a, Iface *b)
{
	USED(s);
	return ifaceeq(*a, *b);
}

static uintptr
nilinterhash(uint32 s, Eface *a)
{
	USED(s);
	return efacehash(*a);
}

static void
nilinterprint(uint32 s, Eface *a)
{
	USED(s);
	·printeface(*a);
}

static uint32
nilinterequal(uint32 s, Eface *a, Eface *b)
{
	USED(s);
	return efaceeq(*a, *b);
}

uintptr
nohash(uint32 s, void *a)
{
	USED(s);
	USED(a);
	panicstring("hash of unhashable type");
	return 0;
}

uint32
noequal(uint32 s, void *a, void *b)
{
	USED(s);
	USED(a);
	USED(b);
	panicstring("comparing uncomparable types");
	return 0;
}

Alg
algarray[] =
{
[AMEM]	{ memhash, memequal, memprint, memcopy },
[ANOEQ]	{ nohash, noequal, memprint, memcopy },
[ASTRING]	{ strhash, strequal, strprint, memcopy },
[AINTER]		{ interhash, interequal, interprint, memcopy },
[ANILINTER]	{ nilinterhash, nilinterequal, nilinterprint, memcopy },
[AMEMWORD] { memhash, memwordequal, memprint, memwordcopy },
};

#pragma textflag 7
void
FLUSH(void *v)
{
	USED(v);
}

int64
nanotime(void)
{
	int64 sec;
	int32 usec;

	sec = 0;
	usec = 0;
	gettime(&sec, &usec);
	return sec*1000000000 + (int64)usec*1000;
}

void
·Caller(int32 skip, uintptr retpc, String retfile, int32 retline, bool retbool)
{
	Func *f;

	if(callers(1+skip, &retpc, 1) == 0 || (f = findfunc(retpc-1)) == nil) {
		retfile = emptystring;
		retline = 0;
		retbool = false;
	} else {
		retfile = f->src;
		retline = funcline(f, retpc-1);
		retbool = true;
	}
	FLUSH(&retfile);
	FLUSH(&retline);
	FLUSH(&retbool);
}

void
·Callers(int32 skip, Slice pc, int32 retn)
{
	retn = callers(skip, (uintptr*)pc.array, pc.len);
	FLUSH(&retn);
}

void
·FuncForPC(uintptr pc, void *retf)
{
	retf = findfunc(pc);
	FLUSH(&retf);
}
