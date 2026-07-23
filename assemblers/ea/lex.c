/*s: ea/lex.c */
#include "a.h"
#include "y.tab.h"

/*
 * claude: modeled on assemblers/5a/lex.c. cinit/syminit/yylex0/yylex
 * are copied close to verbatim (they're generic lexer machinery, not
 * arch-specific); only itab[] (the mnemonic table) and nullgen's field
 * list (Gen has no `reg` here -- see a.h) are wasm-specific.
 */

struct Itab
{
    char	*name;
    ushort	type;
    ushort	value;
};

struct Itab itab[] =
{
    "NOP",		LNULL,		ANOP,
    "UNREACHABLE",	LNULL,		AUNREACHABLE,
    "BLOCK",		LBLOCKOPEN,	ABLOCK,
    "LOOP",		LBLOCKOPEN,	ALOOP,
    "IF",		LBLOCKOPEN,	AIF,
    "ELSE",		LNULL,		AELSE,
    "END",		LENDCTL,	AENDCTL,
    "BR",		LBR,		ABR,
    "BRIF",		LBR,		ABRIF,
    "RET",		LNULL,		ARET,
    "CALL",		LCALL,		ACALL,
    "DROP",		LNULL,		ADROP,
    "SELECT",		LNULL,		ASELECT,
    "TEE",		LTEE,		ALOCALTEE,
    "MEMSIZE",		LNULL,		AMEMSIZE,
    "MEMGROW",		LNULL,		AMEMGROW,

    "MOVB",		LMOV,		AMOVB,
    "MOVBU",		LMOV,		AMOVBU,
    "MOVH",		LMOV,		AMOVH,
    "MOVHU",		LMOV,		AMOVHU,
    "MOVW",		LMOV,		AMOVW,
    "MOVQ",		LMOV,		AMOVQ,
    "MOVF",		LMOV,		AMOVF,
    "MOVD",		LMOV,		AMOVD,
    "MOVBQ",		LMOV,		AMOVBQ,
    "MOVBUQ",		LMOV,		AMOVBUQ,
    "MOVHQ",		LMOV,		AMOVHQ,
    "MOVHUQ",		LMOV,		AMOVHUQ,
    "MOVWQ",		LMOV,		AMOVWQ,
    "MOVWUQ",		LMOV,		AMOVWUQ,
    "MOVQW",		LMOV,		AMOVQW,

    "ADDW",		LNULL,		AADDW,
    "SUBW",		LNULL,		ASUBW,
    "MULW",		LNULL,		AMULW,
    "DIVW",		LNULL,		ADIVW,
    "DIVWU",		LNULL,		ADIVWU,
    "REMW",		LNULL,		AREMW,
    "REMWU",		LNULL,		AREMWU,
    "ANDW",		LNULL,		AANDW,
    "ORW",		LNULL,		AORW,
    "XORW",		LNULL,		AXORW,
    "SHLW",		LNULL,		ASHLW,
    "SHRW",		LNULL,		ASHRW,
    "SHRWU",		LNULL,		ASHRWU,
    "ROLW",		LNULL,		AROLW,
    "RORW",		LNULL,		ARORW,
    "CLZW",		LNULL,		ACLZW,
    "CTZW",		LNULL,		ACTZW,
    "POPCNTW",		LNULL,		APOPCNTW,
    "TESTW",		LNULL,		ATESTW,
    "CMPEQW",		LNULL,		ACMPEQW,
    "CMPNEW",		LNULL,		ACMPNEW,
    "CMPLTW",		LNULL,		ACMPLTW,
    "CMPLTWU",		LNULL,		ACMPLTWU,
    "CMPGTW",		LNULL,		ACMPGTW,
    "CMPGTWU",		LNULL,		ACMPGTWU,
    "CMPLEW",		LNULL,		ACMPLEW,
    "CMPLEWU",		LNULL,		ACMPLEWU,
    "CMPGEW",		LNULL,		ACMPGEW,
    "CMPGEWU",		LNULL,		ACMPGEWU,

    "ADDQ",		LNULL,		AADDQ,
    "SUBQ",		LNULL,		ASUBQ,
    "MULQ",		LNULL,		AMULQ,
    "DIVQ",		LNULL,		ADIVQ,
    "DIVQU",		LNULL,		ADIVQU,
    "REMQ",		LNULL,		AREMQ,
    "REMQU",		LNULL,		AREMQU,
    "ANDQ",		LNULL,		AANDQ,
    "ORQ",		LNULL,		AORQ,
    "XORQ",		LNULL,		AXORQ,
    "SHLQ",		LNULL,		ASHLQ,
    "SHRQ",		LNULL,		ASHRQ,
    "SHRQU",		LNULL,		ASHRQU,
    "ROLQ",		LNULL,		AROLQ,
    "RORQ",		LNULL,		ARORQ,
    "CLZQ",		LNULL,		ACLZQ,
    "CTZQ",		LNULL,		ACTZQ,
    "POPCNTQ",		LNULL,		APOPCNTQ,
    "TESTQ",		LNULL,		ATESTQ,
    "CMPEQQ",		LNULL,		ACMPEQQ,
    "CMPNEQ",		LNULL,		ACMPNEQ,
    "CMPLTQ",		LNULL,		ACMPLTQ,
    "CMPLTQU",		LNULL,		ACMPLTQU,
    "CMPGTQ",		LNULL,		ACMPGTQ,
    "CMPGTQU",		LNULL,		ACMPGTQU,
    "CMPLEQ",		LNULL,		ACMPLEQ,
    "CMPLEQU",		LNULL,		ACMPLEQU,
    "CMPGEQ",		LNULL,		ACMPGEQ,
    "CMPGEQU",		LNULL,		ACMPGEQU,

    "ADDF",		LNULL,		AADDF,
    "SUBF",		LNULL,		ASUBF,
    "MULF",		LNULL,		AMULF,
    "DIVF",		LNULL,		ADIVF,
    "MINF",		LNULL,		AMINF,
    "MAXF",		LNULL,		AMAXF,
    "COPYSGNF",		LNULL,		ACOPYSGNF,
    "ABSF",		LNULL,		AABSF,
    "NEGF",		LNULL,		ANEGF,
    "SQRTF",		LNULL,		ASQRTF,
    "CEILF",		LNULL,		ACEILF,
    "FLOORF",		LNULL,		AFLOORF,
    "TRUNCF",		LNULL,		ATRUNCF,
    "NEARF",		LNULL,		ANEARF,
    "CMPEQF",		LNULL,		ACMPEQF,
    "CMPNEF",		LNULL,		ACMPNEF,
    "CMPLTF",		LNULL,		ACMPLTF,
    "CMPGTF",		LNULL,		ACMPGTF,
    "CMPLEF",		LNULL,		ACMPLEF,
    "CMPGEF",		LNULL,		ACMPGEF,

    "ADDD",		LNULL,		AADDD,
    "SUBD",		LNULL,		ASUBD,
    "MULD",		LNULL,		AMULD,
    "DIVD",		LNULL,		ADIVD,
    "MIND",		LNULL,		AMIND,
    "MAXD",		LNULL,		AMAXD,
    "COPYSGND",		LNULL,		ACOPYSGND,
    "ABSD",		LNULL,		AABSD,
    "NEGD",		LNULL,		ANEGD,
    "SQRTD",		LNULL,		ASQRTD,
    "CEILD",		LNULL,		ACEILD,
    "FLOORD",		LNULL,		AFLOORD,
    "TRUNCD",		LNULL,		ATRUNCD,
    "NEARD",		LNULL,		ANEARD,
    "CMPEQD",		LNULL,		ACMPEQD,
    "CMPNED",		LNULL,		ACMPNED,
    "CMPLTD",		LNULL,		ACMPLTD,
    "CMPGTD",		LNULL,		ACMPGTD,
    "CMPLED",		LNULL,		ACMPLED,
    "CMPGED",		LNULL,		ACMPGED,

    "MOVWF",		LMOV,		AMOVWF,
    "MOVWUF",		LMOV,		AMOVWUF,
    "MOVWD",		LMOV,		AMOVWD,
    "MOVWUD",		LMOV,		AMOVWUD,
    "MOVQF",		LMOV,		AMOVQF,
    "MOVQUF",		LMOV,		AMOVQUF,
    "MOVQD",		LMOV,		AMOVQD,
    "MOVQUD",		LMOV,		AMOVQUD,
    "MOVFW",		LMOV,		AMOVFW,
    "MOVFWU",		LMOV,		AMOVFWU,
    "MOVFQ",		LMOV,		AMOVFQ,
    "MOVFQU",		LMOV,		AMOVFQU,
    "MOVDW",		LMOV,		AMOVDW,
    "MOVDWU",		LMOV,		AMOVDWU,
    "MOVDQ",		LMOV,		AMOVDQ,
    "MOVDQU",		LMOV,		AMOVDQU,
    "MOVFD",		LMOV,		AMOVFD,
    "MOVDF",		LMOV,		AMOVDF,

    "REINTWF",		LMOV,		AREINTWF,
    "REINTFW",		LMOV,		AREINTFW,
    "REINTQD",		LMOV,		AREINTQD,
    "REINTDQ",		LMOV,		AREINTDQ,

    "TEXT",		LDEF,		ATEXT,
    "GLOBL",		LDEF,		AGLOBL,
    "DATA",		LDATA,		ADATA,
    "WORD",		LWORD,		AWORD,

    "LOCAL",		LLOCAL,		0,
    "GLOBAL",		LGLOBAL,	0,
    "SP",		LSP,		D_AUTO,
    "FP",		LFP,		D_PARAM,
    "SB",		LSB,		D_EXTERN,
    0
};

void
cinit(void)
{
    Sym *s;
    int i;

    nullgen.type = D_NONE;
    nullgen.name = D_NONE;
    nullgen.sym = S;
    nullgen.offset = 0;
    nullgen.vval = 0;
    if(FPCHIP)
        nullgen.dval = 0;
    for(i=0; i<sizeof(nullgen.sval); i++)
        nullgen.sval[i] = 0;

    for(i=0; i<NHASH; i++)
        hash[i] = S;
    for(i=0; itab[i].name; i++) {
        s = slookup(itab[i].name);
        s->value = itab[i].value;
        s->type = itab[i].type;
    }

    pathname = allocn(pathname, 0, 100);
    if(getwd(pathname, 99) == 0) {
        pathname = allocn(pathname, 100, 900);
        if(getwd(pathname, 999) == 0)
            strcpy(pathname, "/???");
    }
}

void
syminit(Sym *sym)
{
    sym->type = LNAME;
    sym->value = 0;
}

/*
 * claude: generic lexer body, copied from assemblers/5a/lex.c (numbers,
 * strings, identifiers/keywords, comments) -- nothing here is
 * wasm-specific. See that file's own comments for the history of the
 * ncu:/usuffix handling.
 */
long
yylex0(void)
{
    int c;
    int c1;
    char *cp;
    Sym *s;
    int baselog2;
    int i;

    c = peekc;
    if(c != IGN) {
        peekc = IGN;
        goto l1;
    }
l0:
    c = GETC();
l1:
    if(c == EOF) {
        return EOF;
    }

    if(isspace(c)) {
        if(c == '\n') {
            lineno++;
            return ';';
        }
        goto l0;
    }

    if(isalpha(c))
        goto talph;
    if(isdigit(c))
        goto tnum;
    switch(c) {
    case '/':
        c1 = GETC();
        if(c1 == '/') {
            for(;;) {
                c = GETC();
                if(c == '\n')
                    goto l1;
                if(c == EOF) {
                    yyerror("eof in comment");
                    errorexit();
                }
            }
        }
        if(c1 == '*') {
            for(;;) {
                c = GETC();
                while(c == '*') {
                    c = GETC();
                    if(c == '/')
                        goto l0;
                }
                if(c == EOF) {
                    yyerror("eof in comment");
                    errorexit();
                }
                if(c == '\n')
                    lineno++;
            }
        }
        break;
    case '_':
    case '@':
    talph:
        cp = symb;

    aloop:
        *cp++ = c;
        c = GETC();
        if(isalpha(c) || isdigit(c) || c == '_' || c == '$')
            goto aloop;
        peekc = c;

        *cp = '\0';
        s = lookup();
        if(s->macro) {
            newio();
            cp = ionext->b;
            macexpand(s, cp);
            pushio();

            ionext->link = iostack;
            iostack = ionext;

            fi.p = cp;
            fi.c = strlen(cp);
            if(peekc != IGN) {
                cp[fi.c++] = peekc;
                cp[fi.c] = 0;
                peekc = IGN;
            }
            goto l0;
        }

        if(s->type == LNAME || s->type == LVAR) {
            yylval.sym = s;
        } else {
            yylval.lval = s->value;
        }
        return s->type;
    tnum:
        cp = symb;
        if(c != '0')
            goto dc;
        *cp++ = c;
        c = GETC();
        baselog2 = 3;
        if(c == 'x' || c == 'X') {
            baselog2 = 4;
            c = GETC();
        }
        else if(c < '0' || c > '7')
            goto dc;

        yylval.lval = 0;
        for(;;) {
            if(c >= '0' && c <= '9') {
                if(c > '7' && baselog2 == 3)
                    break;
                yylval.lval <<= baselog2;
                yylval.lval += c - '0';
                c = GETC();
                continue;
            }
            if(baselog2 == 3)
                break;
            if(c >= 'A' && c <= 'F')
                c += 'a' - 'A';
            if(c >= 'a' && c <= 'f') {
                yylval.lval <<= baselog2;
                yylval.lval += c - 'a' + 10;
                c = GETC();
                continue;
            }
            break;
        }
        while(c == 'U' || c == 'u' || c == 'l' || c == 'L')
            c = GETC();
        peekc = c;
        return LCONST;

    dc:
        for(;;) {
            if(!isdigit(c))
                break;
            *cp++ = c;
            c = GETC();
        }
        if(c == '.')
            goto casedot;
        if(c == 'e' || c == 'E')
            goto casee;

        *cp = '\0';
        yylval.lval = strtol(symb, nil, 10);

        while(c == 'U' || c == 'u' || c == 'l' || c == 'L')
            c = GETC();
        peekc = c;
        return LCONST;
    casedot:
        for(;;) {
            *cp++ = c;
            c = GETC();
            if(!isdigit(c))
                break;
        }
        if(c == 'e' || c == 'E')
            goto casee;
        goto caseout;

    casee:
        *cp++ = 'e';
        c = GETC();
        if(c == '+' || c == '-') {
            *cp++ = c;
            c = GETC();
        }
        while(isdigit(c)) {
            *cp++ = c;
            c = GETC();
        }

    caseout:
        *cp = '\0';
        peekc = c;
        if(FPCHIP) {
            yylval.dval = atof(symb);
            return LFCONST;
        } else {
            yyerror("assembler cannot interpret fp constants");
            yylval.lval = 1L;
            return LCONST;
        }
    case '.':
        c = GETC();
        if(isalpha(c)) {
            cp = symb;
            *cp++ = '.';
            goto aloop;
        }
        if(isdigit(c)) {
            cp = symb;
            *cp++ = '.';
            goto casedot;
        }
        peekc = c;
        return '.';
    case '\'':
        c = escchar('\'');
        if(c == EOF)
            c = '\'';
        if(escchar('\'') != EOF)
            yyerror("missing '");

        yylval.lval = c;
        return LCONST;
    case '"':
        memcpy(yylval.sval, nullgen.sval, sizeof(yylval.sval));
        cp = yylval.sval;
        i = 0;
        for(;;) {
            c = escchar('"');
            if(c == EOF)
                break;
            if(i < sizeof(yylval.sval))
                *cp++ = c;
            i++;
        }
        if(i > sizeof(yylval.sval))
            yyerror("string constant too long");
        return LSCONST;
    case '#':
        domacro();
        goto l0;
    default:
        return c;
    }
    peekc = c1;
    return c;
}

int32 stmtline;

long
yylex(void)
{
    long t;

    t = yylex0();
    if(t != ';')
        stmtline = lineno;
    return t;
}
/*e: ea/lex.c */
