%{
#include "a.h"

/*
 * claude: block/loop/if nesting, tracked only while parsing a single
 * function. Unlike every other arch's branch targets -- which are PC
 * displacements the *linker* resolves in a second pass over the whole
 * program -- a wasm branch target is a label depth, a purely lexical
 * property of the function being assembled right now: how many
 * enclosing block/loop/if constructs the reference is nested inside.
 * By the time outcode() is called, the depth is already known; there
 * is nothing left for el to do with it. So this stack lives here, in
 * the assembler, instead of in obj.c/l.h the way real relocations do.
 * There is deliberately no PC pseudo-register for branch targets the
 * way arm/x86/riscv have one (`con(PC)`): a wasm target is nesting-
 * relative, not PC-relative, and spelling it as "PC" would suggest an
 * addressing mode that doesn't exist here.
 */
#define	MAXLABELDEPTH	128
static	Sym*	labelstack[MAXLABELDEPTH];
static	int	nlabelstack;

static void
pushlabel(Sym *s)
{
    if(nlabelstack >= MAXLABELDEPTH) {
        yyerror("blocks nested too deeply");
        return;
    }
    labelstack[nlabelstack++] = s;
}

static void
poplabel(void)
{
    if(nlabelstack <= 0) {
        yyerror("unmatched ENDCTL");
        return;
    }
    nlabelstack--;
}

static int32
labeldepth(Sym *s)
{
    int i;

    for(i = nlabelstack-1; i >= 0; i--)
        if(labelstack[i] == s)
            return (nlabelstack-1) - i;
    yyerror("undefined label: %s", s->name);
    return 0;
}

/*
 * claude: ALOADx/ASTOREx only take the memarg offset -- the base
 * address is a stack value, not part of their encoding (see e.out.h's
 * comment). This is what lets LOADW/STOREW still be written with the
 * familiar `name(SB)`/`off(SP)`/`off(FP)` syntax anyway: it pushes the
 * base address itself (an ACONSTW-with-symbol for SB, since that
 * address is a link-time constant; an AGLOBALGET(SPGLOBAL) for SP/FP,
 * since the shadow stack's base is only known at run time) and
 * returns the offset the actual ALOADx/ASTOREx should carry -- 0 when
 * the whole address was already folded into the constant, or the
 * user's own offset when the base came from SPGLOBAL.
 */
static long
pushaddr(Gen *a)
{
    Gen g;

    switch(a->name) {
    case D_EXTERN:
    case D_STATIC:
        g = *a;
        g.type = D_CONST;
        outcode(ACONSTW, &nullgen, NOREG, &g);
        return 0;
    case D_AUTO:
    case D_PARAM:
        g = nullgen;
        g.type = D_GLOBAL;
        g.offset = SPGLOBAL;
        outcode(AGLOBALGET, &nullgen, NOREG, &g);
        return a->offset;
    }
    yyerror("bad address kind %d", a->name);
    return 0;
}
%}
%union
{
    Sym*	sym;
    vlong	lval;
    double	dval;
    char	sval[NSNAME];
    Gen		gen;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LNULL LBLOCKOPEN LENDCTL LBR LCALL
%token	<lval>	LLOCALOP LGLOBALOP LCONSTI LCONSTFOP LLOAD LSTORE
%token	<lval>	LDEF LDATA LWORD
%token		LLOCAL LGLOBAL LSP LFP LSB
%token	<lval>	LCONST
%token	<sval>	LSCONST
%token	<dval>	LFCONST
%token	<sym>	LNAME LVAR
%type	<lval>	con expr offset
%type	<gen>	ximm fimm addr name oreg rel
%%
prog:
|	prog line

line:
	LNAME '=' expr ';'
	{
		$1->type = LVAR;
		$1->value = $3;
	}
|	LVAR '=' expr ';'
	{
		if($1->value != $3)
			yyerror("redeclaration of %s", $1->name);
		$1->value = $3;
	}
|	';'
|	inst ';'
|	error ';'

/*
 * claude: no LCALLIND (call_indirect) or LBRTABLE (br_table) rules
 * yet -- hello_wasm.s needs neither, and both need a bit more design
 * (a type-section index for the former, a target-label list for the
 * latter) than the rest of this file. The opcodes exist in e.out.h
 * for when that's worth doing.
 */
inst:
	LNULL
	{
		outcode($1, &nullgen, NOREG, &nullgen);
	}
|	LBLOCKOPEN
	{
		pushlabel(S);
		outcode($1, &nullgen, NOREG, &nullgen);
	}
|	LBLOCKOPEN LNAME
	{
		pushlabel($2);
		outcode($1, &nullgen, NOREG, &nullgen);
	}
|	LENDCTL
	{
		poplabel();
		outcode($1, &nullgen, NOREG, &nullgen);
	}
|	LBR rel
	{
		outcode($1, &nullgen, NOREG, &$2);
	}
|	LCALL name
	{
		outcode($1, &nullgen, NOREG, &$2);
	}
|	LLOCALOP LLOCAL '(' expr ')'
	{
		Gen g;
		g = nullgen;
		g.type = D_LOCAL;
		g.offset = $4;
		outcode($1, &nullgen, NOREG, &g);
	}
|	LGLOBALOP LGLOBAL '(' expr ')'
	{
		Gen g;
		g = nullgen;
		g.type = D_GLOBAL;
		g.offset = $4;
		outcode($1, &nullgen, NOREG, &g);
	}
|	LCONSTI ximm
	{
		outcode($1, &nullgen, NOREG, &$2);
	}
|	LCONSTFOP fimm
	{
		outcode($1, &nullgen, NOREG, &$2);
	}
|	LLOAD addr
	{
		Gen g;
		g = nullgen;
		g.type = D_CONST;
		g.offset = pushaddr(&$2);
		outcode($1, &nullgen, NOREG, &g);
	}
|	LSTORE addr
	{
		Gen g;
		g = nullgen;
		g.type = D_CONST;
		g.offset = pushaddr(&$2);
		outcode($1, &nullgen, NOREG, &g);
	}
|	LDEF name ',' ximm
	{
		nlabelstack = 0;
		outcode($1, &$2, NOREG, &$4);
	}
|	LDEF name ',' con ',' ximm
	{
		nlabelstack = 0;
		outcode($1, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' ximm
	{
		outcode($1, &$2, $4, &$6);
	}
|	LWORD ximm
	{
		outcode($1, &nullgen, NOREG, &$2);
	}

rel:
	'(' expr ')'
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.offset = $2;
	}
|	LNAME
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.offset = labeldepth($1);
	}

addr:
	oreg
|	name

oreg:
	'(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_AUTO;
		$$.offset = 0;
	}
|	con '(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_AUTO;
		$$.offset = $1;
	}
|	'(' LFP ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_PARAM;
		$$.offset = 0;
	}
|	con '(' LFP ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_PARAM;
		$$.offset = $1;
	}

name:
	LNAME offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_EXTERN;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_STATIC;
		$$.sym = $1;
		$$.offset = $4;
	}

offset:
	{
		$$ = 0;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'-' con
	{
		$$ = -$2;
	}

/*
 * claude: the vlong-overflow check (also used, unmodified, by
 * assemblers/ia/a.y's own `imm` for the same reason on riscv64) is
 * what lets a bare CONSTQ push of a 64-bit constant that doesn't fit
 * in Gen.offset promote itself to D_VCONST; see e.out.h's D_VCONST.
 */
ximm:
	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
		if((vlong)$$.offset != $2){
			$$.type = D_VCONST;
			$$.vval = $2;
		}
	}
|	'$' addr
	{
		$$ = $2;
		$$.type = D_CONST;
	}
|	'$' LSCONST
	{
		$$ = nullgen;
		$$.type = D_SCONST;
		memcpy($$.sval, $2, sizeof($$.sval));
	}

fimm:
	'$' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = $2;
	}
|	'$' '-' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = -$3;
	}

con:
	LCONST
|	LVAR
	{
		$$ = $1->value;
	}
|	'-' con
	{
		$$ = -$2;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'~' con
	{
		$$ = ~$2;
	}
|	'(' expr ')'
	{
		$$ = $2;
	}

expr:
	con
|	expr '+' expr
	{
		$$ = $1 + $3;
	}
|	expr '-' expr
	{
		$$ = $1 - $3;
	}
|	expr '*' expr
	{
		$$ = $1 * $3;
	}
|	expr '/' expr
	{
		$$ = $1 / $3;
	}
|	expr '%' expr
	{
		$$ = $1 % $3;
	}
|	expr '<' '<' expr
	{
		$$ = $1 << $4;
	}
|	expr '>' '>' expr
	{
		$$ = $1 >> $4;
	}
|	expr '&' expr
	{
		$$ = $1 & $3;
	}
|	expr '^' expr
	{
		$$ = $1 ^ $3;
	}
|	expr '|' expr
	{
		$$ = $1 | $3;
	}
