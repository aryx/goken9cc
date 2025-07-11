#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../vl/v.out.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

typedef	struct	Sym	Sym;
typedef	struct	Gen	Gen;
typedef	struct	Io	Io;
typedef	struct	Hist	Hist;

#define	MAXALIGN	7
#define	FPCHIP		1
#define	NSYMB		8192
#define	BUFSIZ		8192
#define	HISTSZ		20
//kengo: #define	NINCLUDE	10
#define	NHUNK		10000
#define	EOF		(-1)
#define	IGN		(-2)
#define	GETC()		((--fi.c < 0)? filbuf(): *fi.p++ & 0xff)
#define	NHASH		503
#define	STRINGSZ	200
#define	NMACRO		10

struct	Sym
{
	Sym*	link;
	char*	macro;
	long	value;
	ushort	type;
	char	*name;
	char	sym;
};
#define	S	((Sym*)0)

EXTERN	struct
{
	char*	p;
	int	c;
} fi;

struct	Io
{
	Io*	link;
	char	b[BUFSIZ];
	char*	p;
	short	c;
	short	f;
};
#define	I	((Io*)0)

EXTERN	struct
{
	Sym*	sym;
	short	type;
} h[NSYM];

struct	Gen
{
	Sym*	sym;
	long	offset;
	short	type;
	short	reg;
	short	name;
	double	dval;
	char	sval[8];
};

struct	Hist
{
	Hist*	link;
	char*	name;
	long	line;
	long	offset;
};
#define	H	((Hist*)0)

enum
{
	CLAST,
	CMACARG,
	CMACRO,
	CPREPROC
};

EXTERN	char	debug[256];
EXTERN	Sym*	hash[NHASH];
EXTERN	char*	Dlist[30];
EXTERN	int	nDlist;
EXTERN	Hist*	ehist;
EXTERN	int	newflag;
EXTERN	Hist*	hist;
EXTERN	char*	hunk;
//kengo: was EXTERN	char*	include[NINCLUDE];
EXTERN	char**	include;
EXTERN	Io*	iofree;
EXTERN	Io*	ionext;
EXTERN	Io*	iostack;
EXTERN	long	lineno;
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	int	nosched;
EXTERN	int	ninclude;

//kengo: new
EXTERN	int32	nsymb;

EXTERN	Gen	nullgen;
EXTERN	char*	outfile;
EXTERN	int	pass;
EXTERN	char*	pathname;
EXTERN	long	pc;
EXTERN	int	peekc;
EXTERN	int	sym;
//kengo: was char	symb[NSYMB]; now dynamically alloc'ed in lexbody
EXTERN	char*	symb;
EXTERN	int	thechar;
EXTERN	char*	thestring;
EXTERN	long	thunk;
EXTERN	Biobuf	obuf;

// kengo: use of int32 instead of long in a few protos below
void*	alloc(int32);
void*	allocn(void*, int32, int32);

//kengo: new
void	ensuresymb(int32);

void	errorexit(void);
void	pushio(void);
void	newio(void);

void	newfile(char*, int);
Sym*	slookup(char*);
Sym*	lookup(void);
void	syminit(Sym*);
int32	yylex(void);
int	getc(void);
int	getnsc(void);
void	unget(int);
int	escchar(int);
void	cinit(void);
void	pinit(char*);
void	cclean(void);
int	isreg(Gen*);
void	outcode(int, Gen*, int, Gen*);
void	zname(char*, int, int);
void	zaddr(Gen*, int);
void	ieeedtod(Ieee*, double);
int	filbuf(void);
Sym*	getsym(void);
void	domacro(void);
void	macund(void);
void	macdef(void);
void	macexpand(Sym*, char*);
void	macinc(void);
void	maclin(void);
void	macprag(void);
void	macif(int);
void	macend(void);
void	outhist(void);
void	dodefine(char*);
void	prfile(int32);
void	linehist(char*, int);
void	gethunk(void);
void	yyerror(char*, ...);
int	yyparse(void);
void	setinclude(char*);
int	assemble(char*);
