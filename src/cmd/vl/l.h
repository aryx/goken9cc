#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../vl/v.out.h"
#include	"../ld/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

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
		int32	u0offset;
		char*	u0sval;
		Ieee*	u0ieee;
	} u0;
	union
	{
		Auto*	u1autom;
		Sym*	u1sym;
	} u1;
	char	type;
	char	reg;
	char	name;
	char	class;
};

#define	offset	u0.u0offset
#define	sval	u0.u0sval
#define	ieee	u0.u0ieee

#define	autom	u1.u1autom
#define	sym	u1.u1sym

struct	Prog
{
	Adr	from;
	Adr	to;
	union
	{
		int32	u0regused;
		Prog*	u0forwd;
	} u0;
	Prog*	cond;
	Prog*	link;
	int32	pc;
	int32	line;
	uchar	mark;
	uchar	optab;
	char	as;
	char	reg;
};
#define	regused	u0.u0regused
#define	forwd	u0.u0forwd

struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	int32	value;
	Sym*	link;
};
struct	Autom
{
	Sym*	asym;
	Auto*	link;
	int32	aoffset;
	short	type;
};
struct	Optab
{
	char	as;
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
	int32	count;
	int32	outof;
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
	SSTRING,

	C_NONE		= 0,
	C_REG,
	C_FREG,
	C_FCREG,
	C_MREG,
	C_HI,
	C_LO,
	C_ZCON,
	C_SCON,
	C_ADD0CON,
	C_AND0CON,
	C_ADDCON,
	C_ANDCON,
	C_UCON,
	C_LCON,
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
	LOAD		= 1<<5,
	FCMP		= 1<<6,
	NOSCHED		= 1<<7,

	//BIG		= 32766,
    //TODO: maybe make it a global and reset to 0 when debug['X']
    BIG         = 0,
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
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	int32	HEADR;			/* length of header */
EXTERN	int	HEADTYPE;		/* type of header */
EXTERN	int32	INITDAT;		/* data location */
EXTERN	int32	INITRND;		/* data round above text location */
EXTERN	int32	INITTEXT;		/* text location */
EXTERN	int32	INITTEXTP;		/* text location (physical) */
EXTERN	char*	INITENTRY;		/* entry point */
EXTERN	int32	autosize;
EXTERN	Biobuf	bso;
EXTERN	int32	bsssize;
EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	int32	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi4[4];	/* for 3l [sic] */
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
EXTERN	int32	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	int32	nhunk;

EXTERN	int32	instoffset;

EXTERN	Opcross	opcross[10];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	int32	pc;
EXTERN	uchar	repop[ALAST];
EXTERN	int32	symsize;
EXTERN	Prog*	textp;
EXTERN	int32	textsize;
EXTERN	int32	thunk;
EXTERN	int	version;
EXTERN	char	xcmp[32][32];
EXTERN	Prog	zprg;
EXTERN	int	dtype;
EXTERN	int	little;

EXTERN	struct
{
	Count	branch;
	Count	fcmp;
	Count	load;
	Count	mfrom;
	Count	page;
	Count	jump;
} nop;

extern	char*	anames[];
extern	Optab	optab[];

#pragma	varargck	type	"A"	int
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
void	addhist(int32, int);
void	addlibpath(char*);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
int32	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
int	cmp(int, int);
void	cput(int32);
int	compound(Prog*);
double	cputime(void);
void	datblk(int32, int32, int);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
int32	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	fileexists(char*);
int	find1(int32, int);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
int32	ieeedtof(Ieee*);
int	isnop(Prog*);
void	ldobj(int, int32, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	llput(vlong);
void	llputl(vlong);
void	lput(int32);
void	lputl(int32);
void	bput(int32);
void	mkfwd(void);
void*	mysbrk(uint32);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(const void*, const void*);
int32	opirr(int);
Optab*	oplook(Prog*);
int32	oprrr(int);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, int32, int);
int32	regoff(Adr*);
int	relinv(int);
int32	rnd(int32, int32);
void	sched(Prog*, Prog*);
void	span(void);
void	strnput(char*, int);
void	undef(void);
void	wput(int32);
void	wputl(int32);
void	xdefine(char*, int, int32);
void	xfol(Prog*);
void	xfol(Prog*);
void	nopstat(char*, Count*);
