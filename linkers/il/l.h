#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"i.out.h"
#include	"../ld/elf.h"

//TODO? factorize with new ld/ld.h
#ifndef	EXTERN
#define	EXTERN	extern
#endif

typedef vlong xlong;

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;
typedef	struct	Oprang	Oprang;
typedef	uchar	Opcross[32][32];
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
		vlong*	u0vval;
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
#define	vval	u0.u0vval

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
	ushort	file;
	int32	value;
	int32	sig;
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
	char	a3;
	char	type;
	char	ctype;
	char	size;
	char	op;
	char	func3;
	short	param;
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
	C_CTLREG,
	C_FREG,
	C_ZREG,
	C_ZCON,
	C_SCON,
	C_UCON,
	C_LCON,
	C_VCON,
	C_FCON,
	C_SACON,
	C_SECON,
	C_LACON,
	C_LECON,
	C_SRCON,
	C_LRCON,
	C_SBRA,
	C_LBRA,
	C_SAUTO,
	C_ZOREG,
	C_SOREG,
	C_LOREG,
	C_LAUTO,
	C_SEXT,
	C_LEXT,
	C_GOK,

	NSCHED		= 20,

/* mark flags */
	FOLL		= 1<<0,
	LABEL		= 1<<1,
	LEAF		= 1<<2,
	SYNC		= 1<<3,
	BRANCH		= 1<<4,
	COMPR		= 1<<5,
	SPASS		= 1<<6,

	BIG		= 2048,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
};

/* Major opcodes */
enum {
	OLOAD,	 OLOAD_FP,  Ocustom_0,	OMISC_MEM, OOP_IMM, OAUIPC, OOP_IMM_32,	O48b,
	OSTORE,	 OSTORE_FP, Ocustom_1,	OAMO,	   OOP,	    OLUI,   OOP_32,	O64b,
	OMADD,	 OMSUB,	    ONMSUB,	ONMADD,	   OOP_FP,  Ores_0, Ocustom_2,	O48b_2,
	OBRANCH, OJALR,	    Ores_1,	OJAL,	   OSYSTEM, Ores_2, Ocustom_3,	O80b
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
EXTERN	xlong	INITDAT;		/* data location */
EXTERN	xlong	INITRND;		/* data round above text location */
EXTERN	xlong	INITTEXT;		/* text location */
EXTERN	xlong	INITTEXTP;		/* text location (physical) */
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
EXTERN	char	inuxi8[8];
EXTERN	Prog*	lastp;
EXTERN	int32	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	int32	nhunk;
EXTERN	Prog	nopalign;
EXTERN	int32	instoffset;
EXTERN	vlong	instoffx;
EXTERN	Opcross	opcross[10];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	int32	pc;
EXTERN	int	ptrsize;
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
extern	char	thechar;

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
void	append(Prog*, Prog*);
void	asmb(void);
int	asmcompressed(Prog*, Optab*, int, int);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
vlong	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
int	cmp(int, int);
int	compound(Prog*);
double	cputime(void);
void	datblk(int32, int32, int);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
vlong	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	find1(int32, int);
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
void	cput(int);
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
int	ocmp(void*, void*);
int32	opirr(int);
Optab*	oplook(Prog*);
int32	oprrr(int);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, vlong, int);
int32	regoff(Adr*);
int		classreg(Adr*);
int	relinv(int);
int relrev(int);
vlong	rnd(vlong, vlong);
void	span(void);
void	strnput(char*, int);
void	undef(void);
int	vconshift(vlong);
void	wput(int32);
void	wputl(int32);
void	xdefine(char*, int, int32);
void	xfol(Prog*);
void	xfol(Prog*);
void	nopstat(char*, Count*);
