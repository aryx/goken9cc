/*s: grep/grep.y */
%{
#include    "grep.h"
%}

/*s: grep grammar union decl */
%union
{
    int   val;
    char* str;
    Re2   re;
}
/*e: grep grammar union decl */
/*s: grep grammar type decl */
%type   <re>    expr prog
%type   <re>    expr0 expr1 expr2 expr3 expr4
/*e: grep grammar type decl */
/*s: grep grammar [[token]] decl */
%token  <str>   LCLASS
%token  <val>   LCHAR
%token      LLPAREN LRPAREN LALT LSTAR LPLUS LQUES
%token      LBEGIN LEND LDOT
%token      LBAD LNEWLINE
/*e: grep grammar [[token]] decl */
%%
/*s: grep grammar */
prog:   /* empty */
    {
        yyerror("empty pattern");
    }
|   expr newlines
    {
        $$.beg = ral(Tend);
        $$.end = $$.beg;
        $$ = re2cat(re2star(re2or(re2char(0x00, '\n'-1), re2char('\n'+1, 0xff))), $$);
        $$ = re2cat($1, $$);
        $$ = re2cat(re2star(re2char(0x00, 0xff)), $$);
        topre = $$;
    }

expr:
    expr0
|   expr newlines expr0 { $$ = re2or($1, $3); }

expr0:
    expr1
|   LSTAR { literal = true; } expr1 { $$ = $3; }

expr1:
    expr2
|   expr1 LALT expr2     { $$ = re2or($1, $3); }

expr2:
    expr3
|   expr2 expr3  { $$ = re2cat($1, $2); }

expr3:
    expr4
|   expr3 LSTAR { $$ = re2star($1); }
|   expr3 LPLUS
    {
        $$.beg = ral(Talt);
        patchnext($1.end, $$.beg);
        $$.beg->alt = $1.beg;
        $$.end = $$.beg;
        $$.beg = $1.beg;
    }
|   expr3 LQUES
    {
        $$.beg = ral(Talt);
        $$.beg->alt = $1.beg;
        $$.end = $1.end;
        appendnext($$.end,  $$.beg);
    }

expr4:
    LCHAR
    {
        $$.beg = ral(Tclass);
        $$.beg->lo = $1;
        $$.beg->hi = $1;
        $$.end = $$.beg;
    }
|   LBEGIN
    {
        $$.beg = ral(Tbegin);
        $$.end = $$.beg;
    }
|   LEND
    {
        $$.beg = ral(Tend);
        $$.end = $$.beg;
    }
|   LDOT   { $$ = re2class("^\n"); }
|   LCLASS { $$ = re2class($1); }
|   LLPAREN expr1 LRPAREN { $$ = $2; }

newlines:
    LNEWLINE
|   newlines LNEWLINE
/*e: grep grammar */
%%

/*s: function [[yyerror]](grep) */
void
yyerror(char *e, ...)
{
    va_list args;

    fprint(STDERR, "grep: ");
    if(filename)
        fprint(STDERR, "%s:%ld: ", filename, lineno);
    else if (pattern)
        fprint(STDERR, "%s: ", pattern);
    va_start(args, e);
    vfprint(STDERR, e, args);
    va_end(args);
    fprint(STDERR, "\n");
    exits("syntax");
}
/*e: function [[yyerror]](grep) */
/*s: function [[yylex]](grep) */
long
yylex(void)
{
    char *q, *eq;
    int c, s;

    if(peekc) {
        s = peekc;
        peekc = 0;
        return s;
    }
    c = getrec();
    if(literal) {
        if(c != 0 && c != '\n') {
            yylval.val = c;
            return LCHAR;
        }
        literal = false;
    }
    switch(c) {
    default:
        yylval.val = c;
        s = LCHAR;
        break;
    case '\\':
        c = getrec();
        yylval.val = c;
        s = LCHAR;
        if(c == '\n')
            s = LNEWLINE;
        break;
    case '[':
        goto getclass;
    case '(':
        s = LLPAREN;
        break;
    case ')':
        s = LRPAREN;
        break;
    case '|':
        s = LALT;
        break;
    case '*':
        s = LSTAR;
        break;
    case '+':
        s = LPLUS;
        break;
    case '?':
        s = LQUES;
        break;
    case '^':
        s = LBEGIN;
        break;
    case '$':
        s = LEND;
        break;
    case '.':
        s = LDOT;
        break;
    case 0:
        peekc = -1;
    case '\n':
        s = LNEWLINE;
        break;
    }
    return s;

getclass:
    q = u.string;
    eq = q + nelem(u.string) - 5;
    c = getrec();
    if(c == '^') {
        q[0] = '^';
        q[1] = '\n';
        q[2] = '-';
        q[3] = '\n';
        q += 4;
        c = getrec();
    }
    for(;;) {
        if(q >= eq)
            error("class too long");
        if(c == ']' || c == 0)
            break;
        if(c == '\\') {
            *q++ = c;
            c = getrec();
            if(c == 0)
                break;
        }
        *q++ = c;
        c = getrec();
    }
    *q = 0;
    if(c == 0)
        return LBAD;
    yylval.str = u.string;
    return LCLASS;
}
/*e: function [[yylex]](grep) */
/*e: grep/grep.y */
