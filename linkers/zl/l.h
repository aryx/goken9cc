#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<obj/z.out.h>
#include	"../lk/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

/* claude: elf.c (linkers/lk/elf.c) calls cput() as a real function call,
 * so this macro (matching 6l/l.h's) has to be visible from l.h, not just
 * as the CPUT() macro asm.c already had locally for its own use. */
#define	cput(c)\
	{ *cbp++ = c;\
	if(--cbc <= 0)\
		cflush(); }

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;
typedef	struct	Oprang	Oprang;
typedef	uchar	Opcross[32][2][32];
typedef	struct	Count	Count;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Adr
{
	union
	{
		vlong	offset;
		char*	sval;
		Ieee*	ieee;
	};
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	char	type;
	char	reg;
	char	name;
	char	class;
};
struct	Prog
{
	Adr	from;
	Adr	to;
	union
	{
		long	regused;
		Prog*	forwd;
	};
	Prog*	cond;
	Prog*	link;
	long	pc;
	long	line;
	uchar	mark;
	uchar	optab;
	uchar	as;
	char	reg;
};
struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	long	value;
	Sym*	link;
};
struct	Autom
{
	Sym*	sym;
	Auto*	link;
	long	offset;
	short	type;
};
struct	Optab
{
	uchar	as;
	char	a1;
	char	a2;
	char	a3;
	char	type;
	char	size;
	char	param;
};
struct	Oprang
{
	Optab*	start;
	Optab*	stop;
};
struct	Count
{
	long	count;
	long	outof;
};

enum
{
	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SLEAF,
	SFILE,
	SCONST,

	C_NONE		= 0,
	C_REG,
	C_FREG,
	C_FCREG,
	C_PREG,
	C_PCC,
	C_ZCON,
	C_BCON,
	C_NCON,
	C_SCON,
	C_UCON,
	C_LCON,
	C_QCON,
	C_SACON,
	C_SECON,
	C_LACON,
	C_LECON,
	C_SBRA,
	C_LBRA,
	C_SAUTO,
	C_SEXT,
	C_LAUTO,
	C_LEXT,
	C_ZOREG,
	C_SOREG,
	C_LOREG,
	C_GOK,

	NSCHED		= 20,

/* mark flags */
	FOLL		= 1<<0,
	LABEL		= 1<<1,
	LEAF		= 1<<2,
	SYNC		= 1<<3,
	BRANCH		= 1<<4,
	LABEL2		= 1<<5,
	COMPARE		= 1<<6,
	NOSCHED		= 1<<7,

	BIG		= 32760,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
};

EXTERN union
{
	struct
	{
		uchar	cbuf[MAXIO];			/* output buffer */
		uchar	xbuf[MAXIO];			/* input buffer */
	};
	char	dbuf[1];
} buf;

EXTERN	long	HEADR;			/* length of header */
EXTERN	int	HEADTYPE;		/* type of header */
EXTERN	long	INITDAT;		/* data location */
EXTERN	long	INITRND;		/* data round above text location */
EXTERN	long	INITTEXT;		/* text location */
EXTERN	long	INITTEXTP;		/* text location (physical) */
EXTERN	char*	INITENTRY;		/* entry point */
EXTERN	long	autosize;
EXTERN	Biobuf	bso;
EXTERN	long	bsssize;
EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi8[8];
EXTERN	char*	noname;
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char*	hunk;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	Prog*	lastp;
EXTERN	long	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	vlong	offset;
EXTERN	Opcross	opcross[7];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	Prog	*prog_divq;
EXTERN	Prog	*prog_divqu;
EXTERN	Prog	*prog_modq;
EXTERN	Prog	*prog_modqu;
EXTERN	Prog	*prog_divl;
EXTERN	Prog	*prog_divlu;
EXTERN	Prog	*prog_modl;
EXTERN	Prog	*prog_modlu;
EXTERN	uchar	repop[ALAST];
EXTERN	long	symsize;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	thunk;
EXTERN	int	version;
EXTERN	char	xcmp[32][32];
EXTERN	Prog	zprg;
EXTERN	int	dtype;

extern	char*	anames[];
extern	Optab	optab[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
int	cmp(int, int);
int	compound(Prog*);
int	conflict(long, long);
double	cputime(void);
void	datblk(long, long);
int	depend(long, long);
void	diag(char*, ...);
Prog *divsubr(int);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	find1(long, int);
void	follow(void);
Prog *genXXX(Prog *, int, Adr*, int, Adr*);
Prog *genRRR(Prog *, int, int, int, int);
Prog *genIRR(Prog *, int, vlong, int, int);
Prog *genstore(Prog *, int, int, vlong, int);
Prog *genload(Prog *, int, vlong, int, int);
Prog *genjmp(Prog *, int, vlong, int);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	initdiv(void);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(int32);
void	lputbe(long);
void	lputl(int32);
void	llput(vlong);
void	llputl(vlong);
void	wput(int32);
void	wputl(int32);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(void*, void*);
long	opcode(int);
Optab*	oplook(Prog*);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
vlong	regoff(Adr*);
long	regused(Prog*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	strnput(char*, int);
void	vlput(vlong);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
