/*
 * ea/a.h -- wasm
 *
 * claude: modeled on assemblers/5a/a.h and assemblers/8a/a.h (the
 * arches already refactored onto the shared assemblers/as/aa.a
 * library), not assemblers/ia/a.h or assemblers/va/a.h, which still
 * duplicate Sym/Io/Hist/Htab locally and predate that refactor.
 */
#include "../as/aa.h"
#include <e.out.h>

typedef	struct	Gen	Gen;

#define	FPCHIP	true

/*
 * claude: no register file, so there is no NREG-equivalent to pass in
 * outcode()'s `reg` parameter for the (overwhelmingly common) case of
 * an instruction with zero or one real operand. NOREG is that
 * sentinel -- see the comment on outcode() below for why the
 * parameter is kept at all.
 */
#define	NOREG	(-1)

/*s: struct [[Gen]](wasm) */
struct	Gen
{
    // enum<operand_kind> (D_* "type" values: D_OREG/D_CONST/D_LOCAL/...)
    short	type;
    // enum<operand_kind> (D_* "name" values: D_EXTERN/D_STATIC/D_AUTO/
    // D_PARAM), meaningful only when type == D_OREG -- see e.out.h
    short	name;

    // generic value: local/global index, branch depth, memory byte
    // offset, immediate constant -- whichever `type` calls for.
    // claude: no separate alignment field for D_OREG memory operands;
    // el emits the type's natural alignment. Add one if a real need
    // for explicit alignment hints ever comes up.
    long	offset;
    vlong	vval;		// D_VCONST (i64.const not fitting in offset)
    double	dval;		// D_FCONST
    char	sval[NSNAME];	// D_SCONST

    // option<ref<Sym>> (owner = hash), for D_EXTERN/D_STATIC
    Sym*	sym;
};
/*e: struct [[Gen]](wasm) */

extern	Gen	nullgen;
extern	int	pass;
extern	char*	pathname;
extern	char*	thestring;
extern	Biobuf	obuf;

// for a.y
long	yylex(void);
void	outcode(int, Gen*, int, Gen*);

int	escchar(int);

// for lexbody
void	setinclude(char*);
void*	allocn(void*, long, long);
void	errorexit(void);
Sym*	slookup(char*);
void	pinit(char*);
void	ieeedtod(Ieee*, double);
void	dodefine(char*);
void	yyerror(char*, ...);
int	yyparse(void);

// for macbody
int	getc(void);
void	unget(int);
void	pushio(void);
void	newio(void);
void	newfile(char*, int);

int	mywait(int*);
int	mycreat(char*, int);
int	systemtype(int);
int	pathchar(void);

// obj.c (for main.c)
void	outhist(void);
