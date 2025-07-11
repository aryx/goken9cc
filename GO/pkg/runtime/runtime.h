// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
 * basic types
 */
typedef	signed char		int8;
typedef	unsigned char		uint8;
typedef	signed short		int16;
typedef	unsigned short		uint16;
typedef	signed int		int32;
typedef	unsigned int		uint32;
typedef	signed long long int	int64;
typedef	unsigned long long int	uint64;
typedef	float			float32;
typedef	double			float64;

#ifdef _64BIT
typedef	uint64		uintptr;
typedef	int64		intptr;
#else
typedef	uint32		uintptr;
typedef int32		intptr;
#endif

/*
 * get rid of C types
 * the / / / forces a syntax error immediately,
 * which will show "last name: XXunsigned".
 */
#define	unsigned		XXunsigned / / /
#define	signed			XXsigned / / /
#define	char			XXchar / / /
#define	short			XXshort / / /
#define	int			XXint / / /
#define	long			XXlong / / /
#define	float			XXfloat / / /
#define	double			XXdouble / / /

typedef	uint8			bool;
typedef	uint8			byte;

enum
{
	true	= 1,
	false	= 0,
};

/*
 * defined macros
 *    you need super-goru privilege
 *    to add this list.
 */
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	nil		((void*)0)


/*
 * common functions and data
 */
int32	strcmp(byte*, byte*);
int32	findnull(byte*);
int32	findnullw(uint16*);
void	dump(byte*, int32);
int32	runetochar(byte*, int32);
int32	charntorune(int32*, uint8*, int32);

/*
 * defined types
 */

typedef	struct	Alg		Alg;
typedef	struct	Func		Func;
typedef	struct	G		G;
typedef	struct	Gobuf		Gobuf;
typedef	struct	Lock		Lock;
typedef	struct	M		M;
typedef	struct	Mem		Mem;
typedef	union	Note		Note;
typedef	struct	Slice		Slice;
typedef	struct	Stktop		Stktop;
typedef	struct	String		String;
typedef	struct	Usema		Usema;
typedef	struct	SigTab		SigTab;
typedef	struct	MCache		MCache;
typedef	struct	Iface		Iface;
typedef	struct	Itab		Itab;
typedef	struct	Eface		Eface;
typedef	struct	Type		Type;
typedef	struct	Defer		Defer;
typedef	struct	Panic		Panic;
typedef	struct	hash		Hmap;
typedef	struct	Hchan		Hchan;
typedef	struct	Complex64	Complex64;
typedef	struct	Complex128	Complex128;

/*
 * per-cpu declaration.
 * "extern register" is a special storage class implemented by 6c, 8c, etc.
 * on machines with lots of registers, it allocates a register that will not be
 * used in generated code.  on the x86, it allocates a slot indexed by a
 * segment register.
 *
 * amd64: allocated downwards from R15
 * x86: allocated upwards from 0(GS)
 * arm: allocated downwards from R10
 *
 * every C file linked into a Go program must include runtime.h
 * so that the C compiler knows to avoid other uses of these registers.
 * the Go compilers know to avoid them.
 */
extern	register	G*	g;
extern	register	M*	m;

/*
 * defined constants
 */
enum
{
	// G status
	//
	// If you add to this list, add to the list
	// of "okay during garbage collection" status
	// in mgc0.c too.
	Gidle,
	Grunnable,
	Grunning,
	Gsyscall,
	Gwaiting,
	Gmoribund,
	Gdead,
	Grecovery,
};

/*
 * structures
 */
struct	Lock
{
	uint32	key;
#ifdef __WINDOWS__
	void*	event;
#else
	uint32	sema;	// for OS X
#endif
};
struct	Usema
{
	uint32	u;
	uint32	k;
};
union	Note
{
	struct {	// Linux
		Lock	lock;
	};
	struct {	// OS X
		int32	wakeup;
		Usema	sema;
	};
};
struct String
{
	byte*	str;
	int32	len;
};
struct Iface
{
	Itab*	tab;
	void*	data;
};
struct Eface
{
	Type*	type;
	void*	data;
};
struct Complex64
{
	float32	real;
	float32	imag;
};
struct Complex128
{
	float64	real;
	float64	imag;
};

struct	Slice
{				// must not move anything
	byte*	array;		// actual data
	uint32	len;		// number of elements
	uint32	cap;		// allocated number of elements
};
struct	Gobuf
{
	// The offsets of these fields are known to (hard-coded in) libmach.
	byte*	sp;
	byte*	pc;
	G*	g;
};
struct	G
{
	byte*	stackguard;	// cannot move - also known to linker, libmach, libcgo
	byte*	stackbase;	// cannot move - also known to libmach, libcgo
	Defer*	defer;
	Panic*	panic;
	Gobuf	sched;
	byte*	stack0;
	byte*	entry;		// initial function
	G*	alllink;	// on allg
	void*	param;		// passed parameter on wakeup
	int16	status;
	int32	goid;
	uint32	selgen;		// valid sudog pointer
	G*	schedlink;
	bool	readyonstop;
	bool	ispanic;
	M*	m;		// for debuggers, but offset not hard-coded
	M*	lockedm;
	int32	sig;
	uintptr	sigcode0;
	uintptr	sigcode1;
};
struct	M
{
	// The offsets of these fields are known to (hard-coded in) libmach.
	G*	g0;		// goroutine with scheduling stack
	void	(*morepc)(void);
	void*	morefp;	// frame pointer for more stack
	Gobuf	morebuf;	// gobuf arg to morestack

	// Fields not known to debuggers.
	uint32	moreframe;	// size arguments to morestack
	uint32	moreargs;
	uintptr	cret;		// return value from C
	uint64	procid;		// for debuggers, but offset not hard-coded
	G*	gsignal;	// signal-handling G
	uint32	tls[8];		// thread-local storage (for 386 extern register)
	Gobuf	sched;	// scheduling stack
	G*	curg;		// current running goroutine
	int32	id;
	int32	mallocing;
	int32	gcing;
	int32	locks;
	int32	nomemprof;
	int32	waitnextg;
	Note	havenextg;
	G*	nextg;
	M*	alllink;	// on allm
	M*	schedlink;
	uint32	machport;	// Return address for Mach IPC (OS X)
	MCache	*mcache;
	G*	lockedg;
	uint64 freg[8];	// Floating point register storage used by ARM software fp routines
#ifdef __WINDOWS__
	void*	gostack;	// bookmark to keep track of go stack during stdcall
#endif
};
struct	Stktop
{
	// The offsets of these fields are known to (hard-coded in) libmach.
	uint8*	stackguard;
	uint8*	stackbase;
	Gobuf	gobuf;
	uint32	args;

	// Frame pointer: where args start in old frame.
	// fp == gobuf.sp except in the case of a reflected
	// function call, which uses an off-stack argument frame.
	uint8*	fp;
	bool	free;	// call stackfree for this frame?
	bool	panic;	// is this frame the top of a panic?
};
struct	Alg
{
	uintptr	(*hash)(uint32, void*);
	uint32	(*equal)(uint32, void*, void*);
	void	(*print)(uint32, void*);
	void	(*copy)(uint32, void*, void*);
};
struct	SigTab
{
	int32	flags;
	int8	*name;
};
enum
{
	SigCatch = 1<<0,
	SigIgnore = 1<<1,
	SigRestart = 1<<2,
	SigQueue = 1<<3,
	SigPanic = 1<<4,
};

// NOTE(rsc): keep in sync with extern.go:/type.Func.
// Eventually, the loaded symbol table should be closer to this form.
struct	Func
{
	String	name;
	String	type;	// go type string
	String	src;	// src file name
	Slice	pcln;	// pc/ln tab for this func
	uintptr	entry;	// entry pc
	uintptr	pc0;	// starting pc, ln for table
	int32	ln0;
	int32	frame;	// stack frame size
	int32	args;	// number of 32-bit in/out args
	int32	locals;	// number of 32-bit locals
};

#ifdef __WINDOWS__
enum {
   Windows = 1
};
#else
enum {
   Windows = 0
};
#endif


/*
 * known to compiler
 */
enum
{
	AMEM,
	ANOEQ,
	ASTRING,
	AINTER,
	ANILINTER,
	AMEMWORD,
	Amax
};


enum {
	Structrnd = sizeof(uintptr)
};

/*
 * deferred subroutine calls
 */
struct Defer
{
	int32	siz;
	byte*	sp;
	byte*	pc;
	byte*	fn;
	Defer*	link;
	byte	args[8];	// padded to actual size
};

/*
 * panics
 */
struct Panic
{
	Eface	arg;		// argument to panic
	byte*	stackbase;	// g->stackbase in panic
	Panic*	link;		// link to earlier panic
	bool	recovered;	// whether this panic is over
};

/*
 * external data
 */
extern	Alg	algarray[Amax];
extern	String	emptystring;
G*	allg;
M*	allm;
int32	goidgen;
extern	int32	gomaxprocs;
extern	int32	panicking;
extern	int32	maxround;
extern	int32	fd;	// usually 1; set to 2 when panicking
extern	int32	gcwaiting;		// gc is waiting to run
int8*	goos;

/*
 * very low level c-called
 */
void	gogo(Gobuf*, uintptr);
void	gogocall(Gobuf*, void(*)(void));
uintptr	gosave(Gobuf*);
void	·lessstack(void);
void	goargs(void);

void	FLUSH(void*);
void*	getu(void);
void	throw(int8*);
void	panicstring(int8*);
uint32	rnd(uint32, uint32);

void	prints(int8*);
void	printf(int8*, ...);
byte*	mchr(byte*, byte, byte*);
void	mcpy(byte*, byte*, uint32);
int32	mcmp(byte*, byte*, uint32);
void	memmove(void*, void*, uint32);

void*	mal(uintptr);
uint32	cmpstring(String, String);
String	catstring(String, String);
String	concatstring(int32, String*);
String	gostring(byte*);
String  gostringn(byte*, int32);
String	gostringnocopy(byte*);
String	gostringw(uint16*);
void	initsig(int32);
int32	gotraceback(void);
void	traceback(uint8 *pc, uint8 *sp, uint8 *lr, G* gp);
void	tracebackothers(G*);

int32	open(byte*, int32, ...);
int32	write(int32, void*, int32);

bool	cas(uint32*, uint32, uint32);
bool	casp(void**, void*, void*);

uint32	xadd(uint32 volatile*, int32);
void	jmpdefer(byte*, void*);
void	exit1(int32);
void	ready(G*);

byte*	getenv(int8*);

int32	atoi(byte*);
void	newosproc(M *m, G *g, void *stk, void (*fn)(void));
void	signalstack(byte*, int32);
G*	malg(int32);
void	minit(void);
Func*	findfunc(uintptr);
int32	funcline(Func*, uint64);
void*	stackalloc(uint32);
void	stackfree(void*);
MCache*	allocmcache(void);
void	mallocinit(void);
bool	ifaceeq(Iface, Iface);
bool	efaceeq(Eface, Eface);
uintptr	ifacehash(Iface);
uintptr	efacehash(Eface);
uintptr	nohash(uint32, void*);
uint32	noequal(uint32, void*, void*);
void*	malloc(uintptr size);
void	free(void *v);
void	addfinalizer(void*, void(*fn)(void*), int32);
void	walkfintab(void (*fn)(void*));
void	runpanic(Panic*);
void*	getcallersp(void*);

void	exit(int32);

void	breakpoint(void);
void	gosched(void);
void	goexit(void);
void	runcgo(void (*fn)(void*), void*);

void	·entersyscall(void);
void	·exitsyscall(void);

void	startcgocallback(G*);
void	endcgocallback(G*);
G*	newproc1(byte*, byte*, int32, int32);
void	siginit(void);
bool	sigsend(int32 sig);
void	gettime(int64*, int32*);
int32	callers(int32, uintptr*, int32);
int64	nanotime(void);
void	panic(int32);

#pragma	varargck	argpos	printf	1

#pragma	varargck	type	"d"	int32
#pragma	varargck	type	"d"	uint32
#pragma	varargck	type	"D"	int64
#pragma	varargck	type	"D"	uint64
#pragma	varargck	type	"x"	int32
#pragma	varargck	type	"x"	uint32
#pragma	varargck	type	"X"	int64
#pragma	varargck	type	"X"	uint64
#pragma	varargck	type	"p"	void*
#pragma	varargck	type	"p"	uintptr
#pragma	varargck	type	"s"	int8*
#pragma	varargck	type	"s"	uint8*
#pragma	varargck	type	"S"	String

// TODO(rsc): Remove. These are only temporary,
// for the mark and sweep collector.
void	stoptheworld(void);
void	starttheworld(void);

/*
 * mutual exclusion locks.  in the uncontended case,
 * as fast as spin locks (just a few user-level instructions),
 * but on the contention path they sleep in the kernel.
 * a zeroed Lock is unlocked (no need to initialize each lock).
 */
void	lock(Lock*);
void	unlock(Lock*);
void	destroylock(Lock*);

/*
 * sleep and wakeup on one-time events.
 * before any calls to notesleep or notewakeup,
 * must call noteclear to initialize the Note.
 * then, any number of threads can call notesleep
 * and exactly one thread can call notewakeup (once).
 * once notewakeup has been called, all the notesleeps
 * will return.  future notesleeps will return immediately.
 */
void	noteclear(Note*);
void	notesleep(Note*);
void	notewakeup(Note*);

/*
 * Redefine methods for the benefit of gcc, which does not support
 * UTF-8 characters in identifiers.
 */
#ifndef __GNUC__
#define runtime_memclr ·memclr
#define runtime_getcallerpc ·getcallerpc
#define runtime_mmap ·mmap
#define runtime_munmap ·munmap
#define runtime_printslice ·printslice
#define runtime_printbool ·printbool
#define runtime_printfloat ·printfloat
#define runtime_printhex ·printhex
#define runtime_printint ·printint
#define runtime_printiface ·printiface
#define runtime_printeface ·printeface
#define runtime_printpc ·printpc
#define runtime_printpointer ·printpointer
#define runtime_printstring ·printstring
#define runtime_printuint ·printuint
#define runtime_printcomplex ·printcomplex
#define runtime_setcallerpc ·setcallerpc
#endif

/*
 * This is consistent across Linux and BSD.
 * If a new OS is added that is different, move this to
 * $GOOS/$GOARCH/defs.h.
 */
#define EACCES		13

/*
 * low level go-called
 */
uint8*	runtime_mmap(byte*, uintptr, int32, int32, int32, uint32);
void	runtime_munmap(uint8*, uintptr);
void	runtime_memclr(byte*, uint32);
void	runtime_setcallerpc(void*, void*);
void*	runtime_getcallerpc(void*);

/*
 * runtime go-called
 */
void	runtime_printbool(bool);
void	runtime_printfloat(float64);
void	runtime_printint(int64);
void	runtime_printiface(Iface);
void	runtime_printeface(Eface);
void	runtime_printstring(String);
void	runtime_printpc(void*);
void	runtime_printpointer(void*);
void	runtime_printuint(uint64);
void	runtime_printhex(uint64);
void	runtime_printslice(Slice);
void	runtime_printcomplex(Complex128);
void	reflect·call(byte*, byte*, uint32);
void	·panic(Eface);
void	·panicindex(void);
void	·panicslice(void);
/*
 * runtime c-called (but written in Go)
 */
void ·newError(String, Eface*);
void	·printany(Eface);
void	·newTypeAssertionError(Type*, Type*, Type*, String*, String*, String*, String*, Eface*);
void	·newErrorString(String, Eface*);

/*
 * wrapped for go users
 */
float64	Inf(int32 sign);
float64	NaN(void);
float32	float32frombits(uint32 i);
uint32	float32tobits(float32 f);
float64	float64frombits(uint64 i);
uint64	float64tobits(float64 f);
float64	frexp(float64 d, int32 *ep);
bool	isInf(float64 f, int32 sign);
bool	isNaN(float64 f);
float64	ldexp(float64 d, int32 e);
float64	modf(float64 d, float64 *ip);
void	semacquire(uint32*);
void	semrelease(uint32*);
String	signame(int32 sig);
int32	gomaxprocsfunc(int32 n);


void	mapassign(Hmap*, byte*, byte*);
void	mapaccess(Hmap*, byte*, byte*, bool*);
struct hash_iter*	mapiterinit(Hmap*);
void	mapiternext(struct hash_iter*);
bool	mapiterkey(struct hash_iter*, void*);
void	mapiterkeyvalue(struct hash_iter*, void*, void*);
Hmap*	makemap(Type*, Type*, int64);

Hchan*	makechan(Type*, int64);
void	chansend(Hchan*, void*, bool*);
void	chanrecv(Hchan*, void*, bool*);
void	chanclose(Hchan*);
bool	chanclosed(Hchan*);
int32	chanlen(Hchan*);
int32	chancap(Hchan*);

void	ifaceE2I(struct InterfaceType*, Eface, Iface*);
