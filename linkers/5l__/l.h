#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../5l_/5.out.h"
#include	"../ld/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

void	addlibpath(char*);
int	fileexists(char*);
char*	findlib(char*);

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
	uchar	as;
	uchar	scond;
	uchar	reg;
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
	uchar	subtype;
	uchar	used;
	ushort	file;
	int32	value;
	int32	sig;
	Sym*	link;
};

#define SIGNINTERN	(1729*325*1729)

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
	char	flag;
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
	SUNDEF,

	SIMPORT,
	SEXPORT,

	LFROM		= 1<<0,
	LTO		= 1<<1,
	LPOOL		= 1<<2,
	V4		= 1<<3,	/* arm v4 arch */
	VFP		= 1<<4,	/* arm vfpv3 floating point */

	C_NONE		= 0,
	C_REG,
	C_REGREG,
	C_SHIFT,
	C_FREG,
	C_PSR,
	C_FCR,

	C_RCON,		/* 0xff rotated */
	C_NCON,		/* ~RCON */
	C_SCON,		/* 0xffff */
	C_LCON,
	C_FCON,

	C_RACON,
	C_LACON,

	C_RECON,
	C_LECON,

	C_SBRA,
	C_LBRA,

	C_HAUTO,	/* halfword insn offset (-0xff to 0xff) */
	C_FAUTO,	/* float insn offset (0 to 0x3fc, word aligned) */
	C_HFAUTO,	/* both H and F */
	C_SAUTO,	/* -0xfff to 0xfff */
	C_LAUTO,

	C_HEXT,
	C_FEXT,
	C_HFEXT,
	C_SEXT,
	C_LEXT,

	C_HOREG,
	C_FOREG,
	C_HFOREG,
	C_SOREG,
	C_ROREG,
	C_SROREG,	/* both S and R */
	C_LOREG,

	C_ADDR,		/* relocatable address */

	C_GOK,

/* mark flags */
	FOLL		= 1<<0,
	LABEL		= 1<<1,
	LEAF		= 1<<2,

	BIG		= (1<<12)-4,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,	/* limit of path elements for history symbols */

	Roffset	= 22,		/* no. bits for offset in relocation address */
	Rindex	= 10,		/* no. bits for index in relocation address */
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

#ifndef COFFCVT

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
EXTERN	char	fnuxi4[4];
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
EXTERN	Opcross	opcross[8];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	int32	pc;
EXTERN	uchar	repop[ALAST];
EXTERN	int32	symsize;
EXTERN	Prog*	textp;
EXTERN	int32	textsize;
EXTERN	int32	thunk;
EXTERN	int	version;
EXTERN	char	xcmp[C_GOK+1][C_GOK+1];
EXTERN	Prog	zprg;
EXTERN	int	dtype;
EXTERN	int	armv4;
EXTERN	int vfp;

EXTERN	int	doexp, dlm;
EXTERN	int	imports, nimports;
EXTERN	int	exports, nexports;
EXTERN	char*	EXPTAB;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

extern	char*	anames[];
extern	Optab	optab[];

void	addpool(Prog*, Adr*);
EXTERN	Prog*	blitrl;
EXTERN	Prog*	elitrl;

void	initdiv(void);
EXTERN	Prog*	prog_div;
EXTERN	Prog*	prog_divu;
EXTERN	Prog*	prog_mod;
EXTERN	Prog*	prog_modu;

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Cconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(int32, int);
void	addlibpath(char*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmdyn(void);
void	asmlc(void);
void	asmout(Prog*, Optab*);
void	asmsym(void);
int32	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
void	ckoff(Sym*, int32);
int	chipfloat(Ieee*);
int	cmp(int, int);
int	compound(Prog*);
double	cputime(void);
void	datblk(int32, int32, int);
void	diag(char*, ...);
void	divsig(void);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
void	dynreloc(Sym*, int32, int);
int32	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
void	export(void);
int	fileexists(char*);
int	find1(int32, int);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
int32	ieeedtof(Ieee*);
void	import(void);
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
void	mkfwd(void);
void*	mysbrk(usize);
void	names(void);
void	nocache(Prog*);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(void*, void*);
int32	opirr(int);
Optab*	oplook(Prog*);
int32	oprrr(int, int);
int32	opvfprrr(int, int);
int32	olr(int32, int, int, int);
int32	olhr(int32, int, int, int);
int32	olrr(int, int, int, int);
int32	olhrr(int, int, int, int);
int32	osr(int, int, int32, int, int);
int32	oshr(int, int32, int, int);
int32	ofsr(int, int, int32, int, int, Prog*);
int32	osrr(int, int, int, int);
int32	oshrr(int, int, int, int);
int32	omvl(Prog*, Adr*, int);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, int32, int);
void	readundefs(char*, int);
int32	regoff(Adr*);
int	relinv(int);
int32	rnd(int32, int32);
void	span(void);
void	strnput(char*, int);
void	undef(void);
void	undefsym(Sym*);
void	wput(int32);
void	wputl(int32);
void	xdefine(char*, int, int32);
void	xfol(Prog*);
void	zerosig(char*);
void	noops(void);
int32	immrot(uint32);
int32	immaddr(int32);
int32	opbra(int, int);

#endif
