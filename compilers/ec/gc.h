/*
 * ec/gc.h -- wasm C compiler backend
 *
 * claude: modeled on compilers/ic/gc.h's shape (Adr/Prog/Case/C1),
 * linking against the shared compilers/cck/{cc.h,pgen.c,pswt.c} like
 * 6c does (not src/cmd/cc's copy, which 7c/ic/vc use instead -- see
 * docs/notes_wasm.txt for why cck was picked).
 *
 * The big structural difference from every other gc.h: there is no
 * register file, so there is no real register allocator to support.
 * Reg/Rgn/Var (the flow-graph register-coloring machinery real
 * backends fill with ~1000 lines in reg.c) are kept only as opaque
 * placeholder types so gc.h's prototypes still type-check; ec's own
 * reg.c makes regopt() a true no-op. See docs/notes_wasm.txt's
 * "Open questions for ec" section for what's still missing (control
 * flow beyond straight-line code, the address-taken-local split).
 */
#include	"../cck/cc.h"
#include	"../../include/common.out.h"
#include	"../../include/e.out.h"

#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_INT		4
#define	SZ_LONG		4
#define	SZ_IND		4	/* wasm32 pointers */
#define	SZ_FLOAT	4
#define	SZ_VLONG	8
#define	SZ_DOUBLE	8
#define	FNX		100

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Case	Case;
typedef	struct	C1	C1;
typedef	struct	Reg	Reg;
typedef	struct	Rgn	Rgn;
typedef	struct	Var	Var;

/* claude: mirrors assemblers/ea/a.h's Gen and linkers/el/l.h's Adr
 * exactly -- ec writes the identical e.out.h wire format directly
 * (see swt.c's outcode()), the same format ea's assembler produces
 * and el already knows how to read. No `reg` field: there is nothing
 * to name. */
struct	Adr
{
	long	offset;
	vlong	vval;
	double	dval;
	char	sval[NSNAME];
	Sym*	sym;
	char	type;	/* D_* "type" */
	char	name;	/* D_* "name" */
	char	etype;
};
#define	A	((Adr*)0)

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog*	link;
	int32	lineno;
	char	as;
	char	reg;	/* wire format's third small-int slot: DATA width, TEXT flags */
};
#define	P	((Prog*)0)

struct	Case
{
	Case*	link;
	vlong	val;
	int32	label;
	char	def;
	char	isv;
};
#define	C	((Case*)0)

struct	C1
{
	vlong	val;
	int32	label;
};

/* claude: opaque placeholders -- see the file comment above. NVAR/NRGN
 * are deliberately not (re)defined here: compilers/cck/cc.h already
 * defines NVAR for its own unrelated bitset-sizing purposes, and
 * nothing in ec actually sizes an array by these (Reg/Rgn/Var are
 * never instantiated), so redefining them would only collide. */
struct	Reg { int32 unused; };
struct	Rgn { int32 unused; };
struct	Var { int32 unused; };

EXTERN	int32	breakpc;
EXTERN	int32	nbreak;
EXTERN	Case*	cases;
EXTERN	Node	constnode;
EXTERN	Node	fconstnode;
EXTERN	Node	vconstnode;
EXTERN	int32	continpc;
EXTERN	int32	curarg;
EXTERN	int32	cursafe;
EXTERN	Prog*	firstp;
EXTERN	Prog*	lastp;
EXTERN	int32	maxargsafe;
EXTERN	int	mnstring;
EXTERN	Node*	nodrat;
EXTERN	Node*	nodret;
EXTERN	Node*	nodsafe;
EXTERN	int32	nrathole;
EXTERN	int32	nstring;
EXTERN	Prog*	p;
EXTERN	int32	pc;
EXTERN	char	string[NSNAME];
EXTERN	Sym*	symrathole;
EXTERN	Node	znode;
EXTERN	Prog	zprog;

/*
 * claude: the wasm replacement for a register file. Every other
 * arch's nodreg()/regalloc()/regret() stamp a Node with a specific
 * hardware register number; ec's stamp `stackresult` instead, meaning
 * "this value is (or goes) on the wasm operand stack" -- there being
 * only one such "location" (unlike REGRET/FREGRET/REGARG naming
 * distinct registers), regalloc()/regret() ignore which register
 * number they're asked for and regfree() is a no-op. cgen()'s own
 * commit step recognizes this sentinel and leaves the value exactly
 * where it already is, which is precisely what ARET (or a plain
 * expression statement whose value is about to be used by an
 * enclosing operator) needs -- see cgen.c.
 */
EXTERN	Node	stackresult;

/*
 * claude: each C local/param variable is given a small integer
 * (assigned by ec's own align(), reusing the frontend's existing
 * xoffset field to carry an index instead of a byte offset -- there
 * is no struct/array support yet needing real byte offsets, see
 * docs/notes_wasm.txt) identifying its wasm local slot. nlocal is the
 * running count for the function currently being compiled, reset by
 * gtext(); ec never needs to *free* a local index (unlike a register)
 * since wasm has no register pressure to relieve.
 */
EXTERN	int32	nlocal;

/*
 * claude: running parameter count for the function currently being
 * compiled, reset by align()'s Aarg0 case (called once per function,
 * right before its parameters are walked) and incremented by Aarg1
 * (called exactly once per parameter). gtext() (sgen.c) reads this
 * once params are done to emit ASIGNATURE -- see e.out.h's comment.
 */
EXTERN	int32	nparams;

/* claude: expected here, not in compilers/cck/cc.h -- matches
 * compilers/ic/gc.h's own EXTERN declaration of the same frontend-
 * used-but-backend-declared global. */
EXTERN	int	suppress;

/*
 * sgen.c
 */
void	noretval(int);
void	xcom(Node*);

/*
 * cgen.c
 */
void	cgen(Node*, Node*);
void	boolgen(Node*, int, Node*);

/*
 * txt.c
 */
void	ginit(void);
void	gclean(void);
void	nextpc(void);
void	gargs(Node*, Node*, Node*);
Node*	nodconst(int32);
Node*	nod32const(vlong);
Node*	nodfconst(double);
Node*	nodgconst(vlong, Type*);
void	nodreg(Node*, Node*, int);
void	regret(Node*, Node*);
void	regalloc(Node*, Node*, Node*);
void	regfree(Node*);
void	naddr(Node*, Adr*);
int	islocal(Node*);
void	gaddr(Node*);
void	lload(Node*);
void	lstore(Node*);
int	loadop(Type*);
int	storeop(Type*);
void	gmove(Node*, Node*);
void	gins(int, Node*, Node*);
void	gopcode(int, Node*);
void	gbranch(int);
void	patch(Prog*, int32);
void	gpseudo(int, Sym*, Node*);

/*
 * swt.c
 */
void	doswit(Node*);
void	swit1(C1*, int, int32, Node*);
void	casf(void);
long	outstring(char*, long);
void	nullwarn(Node*, Node*);
void	gextern(Sym*, Node*, long, long);
void	outcode(void);
void	ieeedtod(Ieee*, double);

/*
 * list.c
 */
void	listinit(void);
int	Pconv(Fmt*);
int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Sconv(Fmt*);
int	Nconv(Fmt*);
int	Bconv(Fmt*);

/*
 * reg.c -- true no-ops, see the file comment above
 */
void	regopt(Prog*);

/*
 * machcap.c
 */
int	machcap(Node*);

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*
