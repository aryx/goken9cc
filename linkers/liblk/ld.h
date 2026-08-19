#include <u.h>
#include <libc.h>
#include	<bio.h>

//old:
// #include	"../ld/elf.h"
// typedef vlong int64;
/* claude: was `typedef size_t usize;` -- relied on a real host
 * <stddef.h> providing size_t, which BOOT/include's own u.h happens to
 * pull in (so every objtype=boot-gcc/boot-clang linker build always
 * compiled fine), but goken's own libc.h has no size_t at all (found
 * self-hosting linkers/7l via objtype=arm64 for the first time -- no
 * linker had been self-hosted before this, so this shared ld.h, used
 * by every linker's own l.h, never hit goken's own compiler until
 * now).
 *
 * usize (this file's own halloc(usize) declaration below, and
 * falloc.c's now-dead malloc()/calloc()/realloc() family) is a
 * BYTE-COUNT passed to this linker's own arena allocator -- the
 * concept size_t names on a real host. But size_t's own real-world
 * convention is to track the ADDRESS-SPACE width, not be unconditionally
 * 64-bit: an allocator can never be asked for an object bigger than
 * the address space can hold, so on an actual 32-bit host (386/arm/mips,
 * all still targets of this tree) size_t is itself only 4 bytes there
 * too -- there is no in-principle 32-bit host needing a 64-bit byte
 * count, since it could never address the result. That is exactly what
 * uintptr (per-arch u.h) already gives: pointer/address-space width on
 * every target, 8 bytes on a 64-bit arch, 4 on a 32-bit one. uvlong
 * (include/core/types.h) is always 64-bit regardless of arch, which
 * would be the wrong, non-idiomatic choice here -- oversized and
 * inconsistent with how size_t actually behaves on a 32-bit host. */
typedef uintptr usize;

/*
 * basic types in all loaders
 */

typedef	struct	Adr	Adr;
typedef	struct	Auto	Auto;
typedef	struct	Count	Count;
typedef	struct	Ieee	Ieee;
typedef	struct	Prog	Prog;
typedef	struct	Sym	Sym;

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Auto
{
	Sym*	asym;
	Auto*	link;
	vlong	aoffset;
	short	type;
};

struct	Count
{
	int32	count;
	int32	outof;
};

enum
{

	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MAXIO		= 8192,
	MAXHIST		= 20,	/* limit of path elements for history symbols */
};

#define SIGNINTERN	(1729*325*1729)	/* signature of internal functions such as _div */

EXTERN union
{
	struct
	{
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	char	debug[128];
EXTERN	char	fnuxi4[4];
EXTERN	char	fnuxi8[8];
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	uchar	inuxi8[8];
EXTERN	char*	thestring;
EXTERN	char	thechar;

EXTERN	int	doexp, dlm;
EXTERN	int	imports, nimports;
EXTERN	int	exports, nexports;
EXTERN	char*	EXPTAB;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

int	Sconv(Fmt*);
void	addhist(int32, int);
void	addlib(char*);
void	addlibpath(char*);
void	addlibroot(void);
vlong	atolwhex(char*);
Prog*	brchain(Prog*);
Prog*	brloop(Prog*);
void	cflush(void);
void	ckoff(Sym*, int32);
void	collapsefrog(Sym*);
void	cput(int);
void	diag(char*, ...);
void	errorexit(void);
double	cputime(void);
void	dodata(void);
void	export(void);
int	find1(int32, int);
char*	findlib(char*);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
int32	hunkspace(void);
uchar*	readsome(int, uchar*, uchar*, uchar*, int);
void* halloc(usize);
void	histtoauto(void);
double	ieeedtod(Ieee*);
int32	ieeedtof(Ieee*);
void	import(void);
int	isobjfile(char*);
void	loadlib(void);
Sym*	lookup(char*, int);
void	mkfwd(void);
void*	mysbrk(uint32);
void	nopstat(char*, Count*);
void	objfile(char*);
void	patch(void);
void	prasm(Prog*);
Prog*	prg(void);
void	readundefs(char*, int);
uchar*	readsome(int, uchar*, uchar*, uchar*, int);
void	readundefs(char*, int);
vlong	rnd(vlong, int32);
void	strnput(char*, int);
void	undef(void);
void	undefsym(Sym*);
void	xdefine(char*, int, vlong);
void	xfol(Prog*);
void	zerosig(char*);

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1
