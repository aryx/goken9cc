/*s: awk/run.c */
/*s: awk copyright lucent */
/****************************************************************
Copyright (C) Lucent Technologies 1997
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name Lucent Technologies or any of
its entities not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/
/*e: awk copyright lucent */
#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <utf.h>
#include "awk.h"
#include "y.tab.h"

#define tempfree(x) if (istemp(x)) tfree(x); else

/*
#undef tempfree

void tempfree(Cell *p) {
    if (p->ctype == OCELL && (p->csub < CUNK || p->csub > CFREE)) {
        WARNING("bad csub %d in Cell %d %s",
            p->csub, p->ctype, p->sval);
    }
    if (istemp(p))
        tfree(p);
}
*/

#ifdef _NFILE
#ifndef FOPEN_MAX
#define FOPEN_MAX _NFILE
#endif
#endif

#ifndef FOPEN_MAX
#define FOPEN_MAX   40  /* max number of open files */
#endif

#ifndef RAND_MAX
#define RAND_MAX    32767   /* all that ansi guarantees */
#endif

jmp_buf env;
extern  int pairstack[];

Node    *winner = NULL; /* root of parse tree */
Cell    *tmps;      /* free temporary cells for execution */

static Cell truecell    ={ OBOOL, BTRUE, 0, 0, 1.0, NUM };
Cell    *True   = &truecell;
static Cell falsecell   ={ OBOOL, BFALSE, 0, 0, 0.0, NUM };
Cell    *False  = &falsecell;
static Cell breakcell   ={ OJUMP, JBREAK, 0, 0, 0.0, NUM };
Cell    *jbreak = &breakcell;
static Cell contcell    ={ OJUMP, JCONT, 0, 0, 0.0, NUM };
Cell    *jcont  = &contcell;
static Cell nextcell    ={ OJUMP, JNEXT, 0, 0, 0.0, NUM };
Cell    *jnext  = &nextcell;
static Cell nextfilecell    ={ OJUMP, JNEXTFILE, 0, 0, 0.0, NUM };
Cell    *jnextfile  = &nextfilecell;
static Cell exitcell    ={ OJUMP, JEXIT, 0, 0, 0.0, NUM };
Cell    *jexit  = &exitcell;
static Cell retcell     ={ OJUMP, JRET, 0, 0, 0.0, NUM };
Cell    *jret   = &retcell;
static Cell tempcell    ={ OCELL, CTEMP, 0, "", 0.0, NUM|STR|DONTFREE };

Node    *curnode = NULL;    /* the node being executed, for debugging */

/* buffer memory management */
int adjbuf(char **pbuf, int *psiz, int minlen, int quantum, char **pbptr,
    char *whatrtn)
/* pbuf:    address of pointer to buffer being managed
 * psiz:    address of buffer size variable
 * minlen:  minimum length of buffer needed
 * quantum: buffer size quantum
 * pbptr:   address of movable pointer into buffer, or 0 if none
 * whatrtn: name of the calling routine if failure should cause fatal error
 *
 * return   0 for realloc failure, !=0 for success
 */
{
    if (minlen > *psiz) {
        char *tbuf;
        int rminlen = quantum ? minlen % quantum : 0;
        int boff = pbptr ? *pbptr - *pbuf : 0;
        /* round up to next multiple of quantum */
        if (rminlen)
            minlen += quantum - rminlen;
        tbuf = (char *) realloc(*pbuf, minlen);
        if (tbuf == NULL) {
            if (whatrtn)
                FATAL("out of memory in %s", whatrtn);
            return 0;
        }
        *pbuf = tbuf;
        *psiz = minlen;
        if (pbptr)
            *pbptr = tbuf + boff;
    }
    return 1;
}

void run(Node *a)   /* execution of parse tree starts here */
{
    extern void stdinit(void);

    stdinit();
    execute(a);
    closeall();
}

Cell *execute(Node *u)  /* execute a node of the parse tree */
{
    int nobj;
    Cell *(*proc)(Node **, int);
    Cell *x;
    Node *a;

    if (u == NULL)
        return(True);
    for (a = u; ; a = a->nnext) {
        curnode = a;
        if (isvalue(a)) {
            x = (Cell *) (a->narg[0]);
            if (isfld(x) && !donefld)
                fldbld();
            else if (isrec(x) && !donerec)
                recbld();
            return(x);
        }
        nobj = a->nobj;
        if (notlegal(nobj)) /* probably a Cell* but too risky to print */
            FATAL("illegal statement");
        proc = proctab[nobj-FIRSTTOKEN];
        x = (*proc)(a->narg, nobj);
        if (isfld(x) && !donefld)
            fldbld();
        else if (isrec(x) && !donerec)
            recbld();
        if (isexpr(a))
            return(x);
        if (isjump(x))
            return(x);
        if (a->nnext == NULL)
            return(x);
        tempfree(x);
    }
}


Cell *program(Node **a, int n)  /* execute an awk program */
{               /* a[0] = BEGIN, a[1] = body, a[2] = END */
    Cell *x;

    if (setjmp(env) != 0)
        goto ex;
    if (a[0]) {     /* BEGIN */
        x = execute(a[0]);
        if (isexit(x))
            return(True);
        if (isjump(x))
            FATAL("illegal break, continue, next or nextfile from BEGIN");
        tempfree(x);
    }
    if (a[1] || a[2])
        while (getrec(&record, &recsize, 1) > 0) {
            x = execute(a[1]);
            if (isexit(x))
                break;
            tempfree(x);
        }
  ex:
    if (setjmp(env) != 0)   /* handles exit within END */
        goto ex1;
    if (a[2]) {     /* END */
        x = execute(a[2]);
        if (isbreak(x) || isnext(x) || iscont(x))
            FATAL("illegal break, continue, next or nextfile from END");
        tempfree(x);
    }
  ex1:
    return(True);
}

struct Frame {  /* stack frame for awk function calls */
    int nargs;  /* number of arguments in this call */
    Cell *fcncell;  /* pointer to Cell for function */
    Cell **args;    /* pointer to array of arguments after execute */
    Cell *retval;   /* return value */
};

#define NARGS   50  /* max args in a call */

struct Frame *frame = NULL; /* base of stack frames; dynamically allocated */
int nframe = 0;     /* number of frames allocated */
struct Frame *fp = NULL;    /* frame pointer. bottom level unused */

Cell *call(Node **a, int n) /* function call.  very kludgy and fragile */
{
    static Cell newcopycell = { OCELL, CCOPY, 0, "", 0.0, NUM|STR|DONTFREE };
    int i, ncall, ndef;
    Node *x;
    Cell *args[NARGS], *oargs[NARGS];   /* BUG: fixed size arrays */
    Cell *y, *z, *fcn;
    char *s;

    fcn = execute(a[0]);    /* the function itself */
    s = fcn->nval;
    if (!isfcn(fcn))
        FATAL("calling undefined function %s", s);
    if (frame == NULL) {
        fp = frame = (struct Frame *) calloc(nframe += 100, sizeof(struct Frame));
        if (frame == NULL)
            FATAL("out of space for stack frames calling %s", s);
    }
    for (ncall = 0, x = a[1]; x != NULL; x = x->nnext)  /* args in call */
        ncall++;
    ndef = (int) fcn->fval;         /* args in defn */
       dprintf( ("calling %s, %d args (%d in defn), fp=%d\n", s, ncall, ndef, (int) (fp-frame)) );
    if (ncall > ndef)
        WARNING("function %s called with %d args, uses only %d",
            s, ncall, ndef);
    if (ncall + ndef > NARGS)
        FATAL("function %s has %d arguments, limit %d", s, ncall+ndef, NARGS);
    for (i = 0, x = a[1]; x != NULL; i++, x = x->nnext) {   /* get call args */
           dprintf( ("evaluate args[%d], fp=%d:\n", i, (int) (fp-frame)) );
        y = execute(x);
        oargs[i] = y;
           dprintf( ("args[%d]: %s %f <%s>, t=%o\n",
               i, y->nval, y->fval, isarr(y) ? "(array)" : y->sval, y->tval) );
        if (isfcn(y))
            FATAL("can't use function %s as argument in %s", y->nval, s);
        if (isarr(y))
            args[i] = y;    /* arrays by ref */
        else
            args[i] = copycell(y);
        tempfree(y);
    }
    for ( ; i < ndef; i++) {    /* add null args for ones not provided */
        args[i] = gettemp();
        *args[i] = newcopycell;
    }
    fp++;   /* now ok to up frame */
    if (fp >= frame + nframe) {
        int dfp = fp - frame;   /* old index */
        frame = (struct Frame *)
            realloc((char *) frame, (nframe += 100) * sizeof(struct Frame));
        if (frame == NULL)
            FATAL("out of space for stack frames in %s", s);
        fp = frame + dfp;
    }
    fp->fcncell = fcn;
    fp->args = args;
    fp->nargs = ndef;   /* number defined with (excess are locals) */
    fp->retval = gettemp();

       dprintf( ("start exec of %s, fp=%d\n", s, (int) (fp-frame)) );
    y = execute((Node *)(fcn->sval));   /* execute body */
       dprintf( ("finished exec of %s, fp=%d\n", s, (int) (fp-frame)) );

    for (i = 0; i < ndef; i++) {
        Cell *t = fp->args[i];
        if (isarr(t)) {
            if (t->csub == CCOPY) {
                if (i >= ncall) {
                    freesymtab(t);
                    t->csub = CTEMP;
                    tempfree(t);
                } else {
                    oargs[i]->tval = t->tval;
                    oargs[i]->tval &= ~(STR|NUM|DONTFREE);
                    oargs[i]->sval = t->sval;
                    tempfree(t);
                }
            }
        } else if (t != y) {    /* kludge to prevent freeing twice */
            t->csub = CTEMP;
            tempfree(t);
        }
    }
    tempfree(fcn);
    if (isexit(y) || isnext(y) || isnextfile(y))
        return y;
    tempfree(y);        /* this can free twice! */
    z = fp->retval;         /* return value */
       dprintf( ("%s returns %g |%s| %o\n", s, getfval(z), getsval(z), z->tval) );
    fp--;
    return(z);
}

Cell *copycell(Cell *x) /* make a copy of a cell in a temp */
{
    Cell *y;

    y = gettemp();
    y->csub = CCOPY;    /* prevents freeing until call is over */
    y->nval = x->nval;  /* BUG? */
    y->sval = x->sval ? tostring(x->sval) : NULL;
    y->fval = x->fval;
    y->tval = x->tval & ~(CON|FLD|REC|DONTFREE);    /* copy is not constant or field */
                            /* is DONTFREE right? */
    return y;
}

Cell *arg(Node **a, int n)  /* nth argument of a function */
{

    n = ptoi(a[0]); /* argument number, counting from 0 */
       dprintf( ("arg(%d), fp->nargs=%d\n", n, fp->nargs) );
    if (n+1 > fp->nargs)
        FATAL("argument #%d of function %s was not supplied",
            n+1, fp->fcncell->nval);
    return fp->args[n];
}

Cell *jump(Node **a, int n) /* break, continue, next, nextfile, return */
{
    Cell *y;

    switch (n) {
    case EXIT:
        if (a[0] != NULL) {
            y = execute(a[0]);
            errorflag = (int) getfval(y);
            tempfree(y);
        }
        longjmp(env, 1);
    case RETURN:
        if (a[0] != NULL) {
            y = execute(a[0]);
            if ((y->tval & (STR|NUM)) == (STR|NUM)) {
                setsval(fp->retval, getsval(y));
                fp->retval->fval = getfval(y);
                fp->retval->tval |= NUM;
            }
            else if (y->tval & STR)
                setsval(fp->retval, getsval(y));
            else if (y->tval & NUM)
                setfval(fp->retval, getfval(y));
            else        /* can't happen */
                FATAL("bad type variable %d", y->tval);
            tempfree(y);
        }
        return(jret);
    case NEXT:
        return(jnext);
    case NEXTFILE:
        nextfile();
        return(jnextfile);
    case BREAK:
        return(jbreak);
    case CONTINUE:
        return(jcont);
    default:    /* can't happen */
        FATAL("illegal jump type %d", n);
    }
    return 0;   /* not reached */
}

Cell *getline(Node **a, int n)  /* get next line from specific input */
{       /* a[0] is variable, a[1] is operator, a[2] is filename */
    Cell *r, *x;
    extern Cell **fldtab;
    FILE *fp;
    char *buf;
    int bufsize = recsize;
    int mode;

    if ((buf = (char *) malloc(bufsize)) == NULL)
        FATAL("out of memory in getline");

    fflush(stdout); /* in case someone is waiting for a prompt */
    r = gettemp();
    if (a[1] != NULL) {     /* getline < file */
        x = execute(a[2]);      /* filename */
        mode = ptoi(a[1]);
        if (mode == '|')        /* input pipe */
            mode = LE;  /* arbitrary flag */
        fp = openfile(mode, getsval(x));
        tempfree(x);
        if (fp == NULL)
            n = -1;
        else
            n = readrec(&buf, &bufsize, fp);
        if (n <= 0) {
            ;
        } else if (a[0] != NULL) {  /* getline var <file */
            x = execute(a[0]);
            setsval(x, buf);
            tempfree(x);
        } else {            /* getline <file */
            setsval(fldtab[0], buf);
            if (is_number(fldtab[0]->sval)) {
                fldtab[0]->fval = atof(fldtab[0]->sval);
                fldtab[0]->tval |= NUM;
            }
        }
    } else {            /* bare getline; use current input */
        if (a[0] == NULL)   /* getline */
            n = getrec(&record, &recsize, 1);
        else {          /* getline var */
            n = getrec(&buf, &bufsize, 0);
            x = execute(a[0]);
            setsval(x, buf);
            tempfree(x);
        }
    }
    setfval(r, (Awkfloat) n);
    free(buf);
    return r;
}

Cell *getnf(Node **a, int n)    /* get NF */
{
    if (donefld == 0)
        fldbld();
    return (Cell *) a[0];
}

Cell *array(Node **a, int n)    /* a[0] is symtab, a[1] is list of subscripts */
{
    Cell *x, *y, *z;
    char *s;
    Node *np;
    char *buf;
    int bufsz = recsize;
    int nsub = strlen(*SUBSEP);

    if ((buf = (char *) malloc(bufsz)) == NULL)
        FATAL("out of memory in array");

    x = execute(a[0]);  /* Cell* for symbol table */
    buf[0] = 0;
    for (np = a[1]; np; np = np->nnext) {
        y = execute(np);    /* subscript */
        s = getsval(y);
        if (!adjbuf(&buf, &bufsz, strlen(buf)+strlen(s)+nsub+1, recsize, 0, 0))
            FATAL("out of memory for %s[%s...]", x->nval, buf);
        strcat(buf, s);
        if (np->nnext)
            strcat(buf, *SUBSEP);
        tempfree(y);
    }
    if (!isarr(x)) {
           dprintf( ("making %s into an array\n", x->nval) );
        if (freeable(x))
            xfree(x->sval);
        x->tval &= ~(STR|NUM|DONTFREE);
        x->tval |= ARR;
        x->sval = (char *) makesymtab(NSYMTAB);
    }
    z = setsymtab(buf, "", 0.0, STR|NUM, (Array *) x->sval);
    z->ctype = OCELL;
    z->csub = CVAR;
    tempfree(x);
    free(buf);
    return(z);
}

Cell *awkdelete(Node **a, int n)    /* a[0] is symtab, a[1] is list of subscripts */
{
    Cell *x, *y;
    Node *np;
    char *s;
    int nsub = strlen(*SUBSEP);

    x = execute(a[0]);  /* Cell* for symbol table */
    if (!isarr(x))
        return True;
    if (a[1] == 0) {    /* delete the elements, not the table */
        freesymtab(x);
        x->tval &= ~STR;
        x->tval |= ARR;
        x->sval = (char *) makesymtab(NSYMTAB);
    } else {
        int bufsz = recsize;
        char *buf;
        if ((buf = (char *) malloc(bufsz)) == NULL)
            FATAL("out of memory in adelete");
        buf[0] = 0;
        for (np = a[1]; np; np = np->nnext) {
            y = execute(np);    /* subscript */
            s = getsval(y);
            if (!adjbuf(&buf, &bufsz, strlen(buf)+strlen(s)+nsub+1, recsize, 0, 0))
                FATAL("out of memory deleting %s[%s...]", x->nval, buf);
            strcat(buf, s); 
            if (np->nnext)
                strcat(buf, *SUBSEP);
            tempfree(y);
        }
        freeelem(x, buf);
        free(buf);
    }
    tempfree(x);
    return True;
}

Cell *intest(Node **a, int n)   /* a[0] is index (list), a[1] is symtab */
{
    Cell *x, *ap, *k;
    Node *p;
    char *buf;
    char *s;
    int bufsz = recsize;
    int nsub = strlen(*SUBSEP);

    ap = execute(a[1]); /* array name */
    if (!isarr(ap)) {
           dprintf( ("making %s into an array\n", ap->nval) );
        if (freeable(ap))
            xfree(ap->sval);
        ap->tval &= ~(STR|NUM|DONTFREE);
        ap->tval |= ARR;
        ap->sval = (char *) makesymtab(NSYMTAB);
    }
    if ((buf = (char *) malloc(bufsz)) == NULL) {
        FATAL("out of memory in intest");
    }
    buf[0] = 0;
    for (p = a[0]; p; p = p->nnext) {
        x = execute(p); /* expr */
        s = getsval(x);
        if (!adjbuf(&buf, &bufsz, strlen(buf)+strlen(s)+nsub+1, recsize, 0, 0))
            FATAL("out of memory deleting %s[%s...]", x->nval, buf);
        strcat(buf, s);
        tempfree(x);
        if (p->nnext)
            strcat(buf, *SUBSEP);
    }
    k = lookup(buf, (Array *) ap->sval);
    tempfree(ap);
    free(buf);
    if (k == NULL)
        return(False);
    else
        return(True);
}


Cell *matchop(Node **a, int n)  /* ~ and match() */
{
    Cell *x, *y;
    char *s, *t;
    int i;
    void *p;

    x = execute(a[1]);  /* a[1] = target text */
    s = getsval(x);
    if (a[0] == 0)      /* a[1] == 0: already-compiled reg expr */
        p = (void *) a[2];
    else {
        y = execute(a[2]);  /* a[2] = regular expr */
        t = getsval(y);
        p = compre(t);
        tempfree(y);
    }
    if (n == MATCHFCN)
        i = pmatch(p, s, s);
    else
        i = match(p, s, s);
    tempfree(x);
    if (n == MATCHFCN) {
        int start = countposn(s, patbeg-s)+1;
        if (patlen < 0)
            start = 0;
        setfval(rstartloc, (Awkfloat) start);
        setfval(rlengthloc, (Awkfloat) countposn(patbeg, patlen));
        x = gettemp();
        x->tval = NUM;
        x->fval = start;
        return x;
    } else if ((n == MATCH && i == 1) || (n == NOTMATCH && i == 0))
        return(True);
    else
        return(False);
}


Cell *boolop(Node **a, int n)   /* a[0] || a[1], a[0] && a[1], !a[0] */
{
    Cell *x, *y;
    int i;

    x = execute(a[0]);
    i = istrue(x);
    tempfree(x);
    switch (n) {
    case BOR:
        if (i) return(True);
        y = execute(a[1]);
        i = istrue(y);
        tempfree(y);
        if (i) return(True);
        else return(False);
    case AND:
        if ( !i ) return(False);
        y = execute(a[1]);
        i = istrue(y);
        tempfree(y);
        if (i) return(True);
        else return(False);
    case NOT:
        if (i) return(False);
        else return(True);
    default:    /* can't happen */
        FATAL("unknown boolean operator %d", n);
    }
    return 0;   /*NOTREACHED*/
}

Cell *relop(Node **a, int n)    /* a[0 < a[1], etc. */
{
    int i;
    Cell *x, *y;
    Awkfloat j;

    x = execute(a[0]);
    y = execute(a[1]);
    if (x->tval&NUM && y->tval&NUM) {
        j = x->fval - y->fval;
        i = j<0? -1: (j>0? 1: 0);
    } else {
        i = strcmp(getsval(x), getsval(y));
    }
    tempfree(x);
    tempfree(y);
    switch (n) {
    case LT:    if (i<0) return(True);
            else return(False);
    case LE:    if (i<=0) return(True);
            else return(False);
    case NE:    if (i!=0) return(True);
            else return(False);
    case EQ:    if (i == 0) return(True);
            else return(False);
    case GE:    if (i>=0) return(True);
            else return(False);
    case GT:    if (i>0) return(True);
            else return(False);
    default:    /* can't happen */
        FATAL("unknown relational operator %d", n);
    }
    return 0;   /*NOTREACHED*/
}

void tfree(Cell *a) /* free a tempcell */
{
    if (freeable(a)) {
           dprintf( ("freeing %s %s %o\n", a->nval, a->sval, a->tval) );
        xfree(a->sval);
    }
    if (a == tmps)
        FATAL("tempcell list is curdled");
    a->cnext = tmps;
    tmps = a;
}

Cell *gettemp(void) /* get a tempcell */
{   int i;
    Cell *x;

    if (!tmps) {
        tmps = (Cell *) calloc(100, sizeof(Cell));
        if (!tmps)
            FATAL("out of space for temporaries");
        for(i = 1; i < 100; i++)
            tmps[i-1].cnext = &tmps[i];
        tmps[i-1].cnext = 0;
    }
    x = tmps;
    tmps = x->cnext;
    *x = tempcell;
    return(x);
}

Cell *indirect(Node **a, int n) /* $( a[0] ) */
{
    Cell *x;
    int m;
    char *s;

    x = execute(a[0]);
    m = (int) getfval(x);
    if (m == 0 && !is_number(s = getsval(x)))   /* suspicion! */
        FATAL("illegal field $(%s), name \"%s\"", s, x->nval);
        /* BUG: can x->nval ever be null??? */
    tempfree(x);
    x = fieldadr(m);
    x->ctype = OCELL;   /* BUG?  why are these needed? */
    x->csub = CFLD;
    return(x);
}

Cell *substr(Node **a, int nnn)     /* substr(a[0], a[1], a[2]) */
{
    int k, m, n;
    char *s, *p;
    int temp;
    Cell *x, *y, *z = 0;

    x = execute(a[0]);
    y = execute(a[1]);
    if (a[2] != 0)
        z = execute(a[2]);
    s = getsval(x);
    k = countposn(s, strlen(s)) + 1;
    if (k <= 1) {
        tempfree(x);
        tempfree(y);
        if (a[2] != 0)
            tempfree(z);
        x = gettemp();
        setsval(x, "");
        return(x);
    }
    m = (int) getfval(y);
    if (m <= 0)
        m = 1;
    else if (m > k)
        m = k;
    tempfree(y);
    if (a[2] != 0) {
        n = (int) getfval(z);
        tempfree(z);
    } else
        n = k - 1;
    if (n < 0)
        n = 0;
    else if (n > k - m)
        n = k - m;
       dprintf( ("substr: m=%d, n=%d, s=%s\n", m, n, s) );
    y = gettemp();
    while (*s && --m)
         s += mblen(s, k);
    for (p = s; *p && n--; p += mblen(p, k))
            ;
    temp = *p;  /* with thanks to John Linderman */
    *p = '\0';
    setsval(y, s);
    *p = temp;
    tempfree(x);
    return(y);
}

Cell *sindex(Node **a, int nnn)     /* index(a[0], a[1]) */
{
    Cell *x, *y, *z;
    char *s1, *s2, *p1, *p2, *q;
    Awkfloat v = 0.0;

    x = execute(a[0]);
    s1 = getsval(x);
    y = execute(a[1]);
    s2 = getsval(y);

    z = gettemp();
    for (p1 = s1; *p1 != '\0'; p1++) {
        for (q=p1, p2=s2; *p2 != '\0' && *q == *p2; q++, p2++)
            ;
        if (*p2 == '\0') {
            v = (Awkfloat) countposn(s1, p1-s1) + 1;    /* origin 1 */
            break;
        }
    }
    tempfree(x);
    tempfree(y);
    setfval(z, v);
    return(z);
}

#define MAXNUMSIZE  50

int format(char **pbuf, int *pbufsize, char *s, Node *a)    /* printf-like conversions */
{
    char *fmt;
    char *p, *t, *os;
    Cell *x;
    int flag = 0, n;
    int fmtwd; /* format width */
    int fmtsz = recsize;
    char *buf = *pbuf;
    int bufsize = *pbufsize;

    os = s;
    p = buf;
    if ((fmt = (char *) malloc(fmtsz)) == NULL)
        FATAL("out of memory in format()");
    while (*s) {
        adjbuf(&buf, &bufsize, MAXNUMSIZE+1+p-buf, recsize, &p, "format");
        if (*s != '%') {
            *p++ = *s++;
            continue;
        }
        if (*(s+1) == '%') {
            *p++ = '%';
            s += 2;
            continue;
        }
        /* have to be real careful in case this is a huge number, eg, %100000d */
        fmtwd = atoi(s+1);
        if (fmtwd < 0)
            fmtwd = -fmtwd;
        adjbuf(&buf, &bufsize, fmtwd+1+p-buf, recsize, &p, "format");
        for (t = fmt; (*t++ = *s) != '\0'; s++) {
            if (!adjbuf(&fmt, &fmtsz, MAXNUMSIZE+1+t-fmt, recsize, &t, 0))
                FATAL("format item %.30s... ran format() out of memory", os);
            if (isalpha(*s) && *s != 'l' && *s != 'h' && *s != 'L')
                break;  /* the ansi panoply */
            if (*s == '*') {
                x = execute(a);
                a = a->nnext;
                sprintf(t-1, "%d", fmtwd=(int) getfval(x));
                if (fmtwd < 0)
                    fmtwd = -fmtwd;
                adjbuf(&buf, &bufsize, fmtwd+1+p-buf, recsize, &p, "format");
                t = fmt + strlen(fmt);
                tempfree(x);
            }
        }
        *t = '\0';
        if (fmtwd < 0)
            fmtwd = -fmtwd;
        adjbuf(&buf, &bufsize, fmtwd+1+p-buf, recsize, &p, "format");

        switch (*s) {
        case 'f': case 'e': case 'g': case 'E': case 'G':
            flag = 1;
            break;
        case 'd': case 'i':
            flag = 2;
            if(*(s-1) == 'l') break;
            *(t-1) = 'l';
            *t = 'd';
            *++t = '\0';
            break;
        case 'o': case 'x': case 'X': case 'u':
            flag = *(s-1) == 'l' ? 2 : 3;
            break;
        case 's':
            flag = 4;
            break;
        case 'c':
            flag = 5;
            break;
        default:
            WARNING("weird printf conversion %s", fmt);
            flag = 0;
            break;
        }
        if (a == NULL)
            FATAL("not enough args in printf(%s)", os);
        x = execute(a);
        a = a->nnext;
        n = MAXNUMSIZE;
        if (fmtwd > n)
            n = fmtwd;
        adjbuf(&buf, &bufsize, 1+n+p-buf, recsize, &p, "format");
        switch (flag) {
        case 0: sprintf(p, "%s", fmt);  /* unknown, so dump it too */
            t = getsval(x);
            n = strlen(t);
            if (fmtwd > n)
                n = fmtwd;
            adjbuf(&buf, &bufsize, 1+strlen(p)+n+p-buf, recsize, &p, "format");
            p += strlen(p);
            sprintf(p, "%s", t);
            break;
        case 1: sprintf(p, fmt, getfval(x)); break;
        case 2: sprintf(p, fmt, (long) getfval(x)); break;
        case 3: sprintf(p, fmt, (int) getfval(x)); break;
        case 4:
            t = getsval(x);
            n = strlen(t);
            if (fmtwd > n)
                n = fmtwd;
            if (!adjbuf(&buf, &bufsize, 1+n+p-buf, recsize, &p, 0))
                FATAL("huge string/format (%d chars) in printf %.30s... ran format() out of memory", n, t);
            sprintf(p, fmt, t);
            break;
        case 5:
            if (isnum(x)) {
                if (getfval(x))
                    sprintf(p, fmt, (int) getfval(x));
                else{
                    *p++ = '\0';
                    *p = '\0';
                }
            } else
                sprintf(p, fmt, getsval(x)[0]);
            break;
        }
        tempfree(x);
        p += strlen(p);
        s++;
    }
    *p = '\0';
    free(fmt);
    for ( ; a; a = a->nnext)        /* evaluate any remaining args */
        execute(a);
    *pbuf = buf;
    *pbufsize = bufsize;
    return p - buf;
}

Cell *awksprintf(Node **a, int n)       /* sprintf(a[0]) */
{
    Cell *x;
    Node *y;
    char *buf;
    int bufsz=3*recsize;

    if ((buf = (char *) malloc(bufsz)) == NULL)
        FATAL("out of memory in awksprintf");
    y = a[0]->nnext;
    x = execute(a[0]);
    if (format(&buf, &bufsz, getsval(x), y) == -1)
        FATAL("sprintf string %.30s... too long.  can't happen.", buf);
    tempfree(x);
    x = gettemp();
    x->sval = buf;
    x->tval = STR;
    return(x);
}

Cell *awkprintf(Node **a, int n)        /* printf */
{   /* a[0] is list of args, starting with format string */
    /* a[1] is redirection operator, a[2] is redirection file */
    FILE *fp;
    Cell *x;
    Node *y;
    char *buf;
    int len;
    int bufsz=3*recsize;

    if ((buf = (char *) malloc(bufsz)) == NULL)
        FATAL("out of memory in awkprintf");
    y = a[0]->nnext;
    x = execute(a[0]);
    if ((len = format(&buf, &bufsz, getsval(x), y)) == -1)
        FATAL("printf string %.30s... too long.  can't happen.", buf);
    tempfree(x);
    if (a[1] == NULL) {
        /* fputs(buf, stdout); */
        fwrite(buf, len, 1, stdout);
        if (ferror(stdout))
            FATAL("write error on stdout");
    } else {
        fp = redirect(ptoi(a[1]), a[2]);
        /* fputs(buf, fp); */
        fwrite(buf, len, 1, fp);
        fflush(fp);
        if (ferror(fp))
            FATAL("write error on %s", filename(fp));
    }
    free(buf);
    return(True);
}

Cell *arith(Node **a, int n)    /* a[0] + a[1], etc.  also -a[0] */
{
    Awkfloat i, j = 0;
    double v;
    Cell *x, *y, *z;

    x = execute(a[0]);
    i = getfval(x);
    tempfree(x);
    if (n != UMINUS) {
        y = execute(a[1]);
        j = getfval(y);
        tempfree(y);
    }
    z = gettemp();
    switch (n) {
    case ADD:
        i += j;
        break;
    case MINUS:
        i -= j;
        break;
    case MULT:
        i *= j;
        break;
    case DIVIDE:
        if (j == 0)
            FATAL("division by zero");
        i /= j;
        break;
    case MOD:
        if (j == 0)
            FATAL("division by zero in mod");
        modf(i/j, &v);
        i = i - j * v;
        break;
    case UMINUS:
        i = -i;
        break;
    case POWER:
        if (j >= 0 && modf(j, &v) == 0.0)   /* pos integer exponent */
            i = ipow(i, (int) j);
        else
            i = errcheck(pow(i, j), "pow");
        break;
    default:    /* can't happen */
        FATAL("illegal arithmetic operator %d", n);
    }
    setfval(z, i);
    return(z);
}

double ipow(double x, int n)    /* x**n.  ought to be done by pow, but isn't always */
{
    double v;

    if (n <= 0)
        return 1;
    v = ipow(x, n/2);
    if (n % 2 == 0)
        return v * v;
    else
        return x * v * v;
}

Cell *incrdecr(Node **a, int n)     /* a[0]++, etc. */
{
    Cell *x, *z;
    int k;
    Awkfloat xf;

    x = execute(a[0]);
    xf = getfval(x);
    k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
    if (n == PREINCR || n == PREDECR) {
        setfval(x, xf + k);
        return(x);
    }
    z = gettemp();
    setfval(z, xf);
    setfval(x, xf + k);
    tempfree(x);
    return(z);
}

Cell *assign(Node **a, int n)   /* a[0] = a[1], a[0] += a[1], etc. */
{       /* this is subtle; don't muck with it. */
    Cell *x, *y;
    Awkfloat xf, yf;
    double v;

    y = execute(a[1]);
    x = execute(a[0]);
    if (n == ASSIGN) {  /* ordinary assignment */
        if (x == y && !(x->tval & (FLD|REC)))   /* self-assignment: */
            ;       /* leave alone unless it's a field */
        else if ((y->tval & (STR|NUM)) == (STR|NUM)) {
            setsval(x, getsval(y));
            x->fval = getfval(y);
            x->tval |= NUM;
        }
        else if (isstr(y))
            setsval(x, getsval(y));
        else if (isnum(y))
            setfval(x, getfval(y));
        else
            funnyvar(y, "read value of");
        tempfree(y);
        return(x);
    }
    xf = getfval(x);
    yf = getfval(y);
    switch (n) {
    case ADDEQ:
        xf += yf;
        break;
    case SUBEQ:
        xf -= yf;
        break;
    case MULTEQ:
        xf *= yf;
        break;
    case DIVEQ:
        if (yf == 0)
            FATAL("division by zero in /=");
        xf /= yf;
        break;
    case MODEQ:
        if (yf == 0)
            FATAL("division by zero in %%=");
        modf(xf/yf, &v);
        xf = xf - yf * v;
        break;
    case POWEQ:
        if (yf >= 0 && modf(yf, &v) == 0.0) /* pos integer exponent */
            xf = ipow(xf, (int) yf);
        else
            xf = errcheck(pow(xf, yf), "pow");
        break;
    default:
        FATAL("illegal assignment operator %d", n);
        break;
    }
    tempfree(y);
    setfval(x, xf);
    return(x);
}

Cell *cat(Node **a, int q)  /* a[0] cat a[1] */
{
    Cell *x, *y, *z;
    int n1, n2;
    char *s;

    x = execute(a[0]);
    y = execute(a[1]);
    getsval(x);
    getsval(y);
    n1 = strlen(x->sval);
    n2 = strlen(y->sval);
    s = (char *) malloc(n1 + n2 + 1);
    if (s == NULL)
        FATAL("out of space concatenating %.15s... and %.15s...",
            x->sval, y->sval);
    strcpy(s, x->sval);
    strcpy(s+n1, y->sval);
    tempfree(y);
    z = gettemp();
    z->sval = s;
    z->tval = STR;
    tempfree(x);
    return(z);
}

Cell *pastat(Node **a, int n)   /* a[0] { a[1] } */
{
    Cell *x;

    if (a[0] == 0)
        x = execute(a[1]);
    else {
        x = execute(a[0]);
        if (istrue(x)) {
            tempfree(x);
            x = execute(a[1]);
        }
    }
    return x;
}

Cell *dopa2(Node **a, int n)    /* a[0], a[1] { a[2] } */
{
    Cell *x;
    int pair;

    pair = ptoi(a[3]);
    if (pairstack[pair] == 0) {
        x = execute(a[0]);
        if (istrue(x))
            pairstack[pair] = 1;
        tempfree(x);
    }
    if (pairstack[pair] == 1) {
        x = execute(a[1]);
        if (istrue(x))
            pairstack[pair] = 0;
        tempfree(x);
        x = execute(a[2]);
        return(x);
    }
    return(False);
}

Cell *split(Node **a, int nnn)  /* split(a[0], a[1], a[2]); a[3] is type */
{
    Cell *x = 0, *y, *ap;
    char *s, *t, *fs = 0;
    char temp, num[50];
    int n, nb, sep, tempstat, arg3type;

    y = execute(a[0]);  /* source string */
    s = getsval(y);
    arg3type = ptoi(a[3]);
    if (a[2] == 0)      /* fs string */
        fs = *FS;
    else if (arg3type == STRING) {  /* split(str,arr,"string") */
        x = execute(a[2]);
        fs = getsval(x);
    } else if (arg3type == REGEXPR)
        fs = "(regexpr)";   /* split(str,arr,/regexpr/) */
    else
        FATAL("illegal type of split");
    sep = *fs;
    ap = execute(a[1]); /* array name */
    freesymtab(ap);
       dprintf( ("split: s=|%s|, a=%s, sep=|%s|\n", s, ap->nval, fs) );
    ap->tval &= ~STR;
    ap->tval |= ARR;
    ap->sval = (char *) makesymtab(NSYMTAB);

    n = 0;
    if ((*s != '\0' && strlen(fs) > 1) || arg3type == REGEXPR) {    /* reg expr */
        void *p;
        if (arg3type == REGEXPR) {  /* it's ready already */
            p = (void *) a[2];
        } else {
            p = compre(fs);
        }
        t = s;
        if (nematch(p,s,t)) {
            do {
                n++;
                sprintf(num, "%d", n);
                temp = *patbeg;
                *patbeg = '\0';
                if (is_number(t))
                    setsymtab(num, t, atof(t), STR|NUM, (Array *) ap->sval);
                else
                    setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
                *patbeg = temp;
                t = patbeg + patlen;
                if (t[-1] == 0 || *t == 0) {
                    n++;
                    sprintf(num, "%d", n);
                    setsymtab(num, "", 0.0, STR, (Array *) ap->sval);
                    goto spdone;
                }
            } while (nematch(p,s,t));
        }
        n++;
        sprintf(num, "%d", n);
        if (is_number(t))
            setsymtab(num, t, atof(t), STR|NUM, (Array *) ap->sval);
        else
            setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
  spdone:
        p = NULL;
    } else if (sep == ' ') {
        for (n = 0; ; ) {
            while (*s == ' ' || *s == '\t' || *s == '\n')
                s++;
            if (*s == 0)
                break;
            n++;
            t = s;
            do
                s++;
            while (*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0');
            temp = *s;
            *s = '\0';
            sprintf(num, "%d", n);
            if (is_number(t))
                setsymtab(num, t, atof(t), STR|NUM, (Array *) ap->sval);
            else
                setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
            *s = temp;
            if (*s != 0)
                s++;
        }
    } else if (sep == 0) {  /* new: split(s, a, "") => 1 char/elem */
        for (n = 0; *s != 0; s += nb) {
            Rune r;
            char buf[UTFmax+1];

            n++;
            snprintf(num, sizeof num, "%d", n);
            nb = chartorune(&r, s);
            memmove(buf, s, nb);
            buf[nb] = '\0';
            if (isdigit(buf[0]))
                setsymtab(num, buf, atof(buf), STR|NUM, (Array *) ap->sval);
            else
                setsymtab(num, buf, 0.0, STR, (Array *) ap->sval);
        }
    } else if (*s != 0) {
        for (;;) {
            n++;
            t = s;
            while (*s != sep && *s != '\n' && *s != '\0')
                s++;
            temp = *s;
            *s = '\0';
            sprintf(num, "%d", n);
            if (is_number(t))
                setsymtab(num, t, atof(t), STR|NUM, (Array *) ap->sval);
            else
                setsymtab(num, t, 0.0, STR, (Array *) ap->sval);
            *s = temp;
            if (*s++ == 0)
                break;
        }
    }
    tempfree(ap);
    tempfree(y);
    if (a[2] != 0 && arg3type == STRING)
        tempfree(x);
    x = gettemp();
    x->tval = NUM;
    x->fval = n;
    return(x);
}

Cell *condexpr(Node **a, int n) /* a[0] ? a[1] : a[2] */
{
    Cell *x;

    x = execute(a[0]);
    if (istrue(x)) {
        tempfree(x);
        x = execute(a[1]);
    } else {
        tempfree(x);
        x = execute(a[2]);
    }
    return(x);
}

Cell *ifstat(Node **a, int n)   /* if (a[0]) a[1]; else a[2] */
{
    Cell *x;

    x = execute(a[0]);
    if (istrue(x)) {
        tempfree(x);
        x = execute(a[1]);
    } else if (a[2] != 0) {
        tempfree(x);
        x = execute(a[2]);
    }
    return(x);
}

Cell *whilestat(Node **a, int n)    /* while (a[0]) a[1] */
{
    Cell *x;

    for (;;) {
        x = execute(a[0]);
        if (!istrue(x))
            return(x);
        tempfree(x);
        x = execute(a[1]);
        if (isbreak(x)) {
            x = True;
            return(x);
        }
        if (isnext(x) || isexit(x) || isret(x))
            return(x);
        tempfree(x);
    }
}

Cell *dostat(Node **a, int n)   /* do a[0]; while(a[1]) */
{
    Cell *x;

    for (;;) {
        x = execute(a[0]);
        if (isbreak(x))
            return True;
        if (isnext(x) || isnextfile(x) || isexit(x) || isret(x))
            return(x);
        tempfree(x);
        x = execute(a[1]);
        if (!istrue(x))
            return(x);
        tempfree(x);
    }
}

Cell *forstat(Node **a, int n)  /* for (a[0]; a[1]; a[2]) a[3] */
{
    Cell *x;

    x = execute(a[0]);
    tempfree(x);
    for (;;) {
        if (a[1]!=0) {
            x = execute(a[1]);
            if (!istrue(x)) return(x);
            else tempfree(x);
        }
        x = execute(a[3]);
        if (isbreak(x))     /* turn off break */
            return True;
        if (isnext(x) || isexit(x) || isret(x))
            return(x);
        tempfree(x);
        x = execute(a[2]);
        tempfree(x);
    }
}

Cell *instat(Node **a, int n)   /* for (a[0] in a[1]) a[2] */
{
    Cell *x, *vp, *arrayp, *cp, *ncp;
    Array *tp;
    int i;

    vp = execute(a[0]);
    arrayp = execute(a[1]);
    if (!isarr(arrayp)) {
        return True;
    }
    tp = (Array *) arrayp->sval;
    tempfree(arrayp);
    for (i = 0; i < tp->size; i++) {    /* this routine knows too much */
        for (cp = tp->tab[i]; cp != NULL; cp = ncp) {
            setsval(vp, cp->nval);
            ncp = cp->cnext;
            x = execute(a[2]);
            if (isbreak(x)) {
                tempfree(vp);
                return True;
            }
            if (isnext(x) || isexit(x) || isret(x)) {
                tempfree(vp);
                return(x);
            }
            tempfree(x);
        }
    }
    return True;
}

Cell *bltin(Node **a, int n)    /* builtin functions. a[0] is type, a[1] is arg list */
{
    Cell *x, *y;
    Awkfloat u;
    int t;
    wchar_t wc;
    char *p, *buf;
    char mbc[50];
    Node *nextarg;
    FILE *fp;
    void flush_all(void);

    t = ptoi(a[0]);
    x = execute(a[1]);
    nextarg = a[1]->nnext;
    switch (t) {
    case FLENGTH:
        if (isarr(x))
            u = ((Array *) x->sval)->nelem; /* GROT. should be function*/
        else {
            p = getsval(x);
            u = (Awkfloat) countposn(p, strlen(p));
        }
        break;
    case FLOG:
        u = errcheck(log(getfval(x)), "log"); break;
    case FINT:
        modf(getfval(x), &u); break;
    case FEXP:
        u = errcheck(exp(getfval(x)), "exp"); break;
    case FSQRT:
        u = errcheck(sqrt(getfval(x)), "sqrt"); break;
    case FSIN:
        u = sin(getfval(x)); break;
    case FCOS:
        u = cos(getfval(x)); break;
    case FATAN:
        if (nextarg == 0) {
            WARNING("atan2 requires two arguments; returning 1.0");
            u = 1.0;
        } else {
            y = execute(a[1]->nnext);
            u = atan2(getfval(x), getfval(y));
            tempfree(y);
            nextarg = nextarg->nnext;
        }
        break;
    case FSYSTEM:
        fflush(stdout);     /* in case something is buffered already */
        u = (Awkfloat) system(getsval(x)) / 256;   /* 256 is unix-dep */
        break;
    case FRAND:
        /* in principle, rand() returns something in 0..RAND_MAX */
        u = (Awkfloat) (rand() % RAND_MAX) / RAND_MAX;
        break;
    case FSRAND:
        if (isrec(x))   /* no argument provided */
            u = time((time_t *)0);
        else
            u = getfval(x);
        srand((unsigned int) u);
        break;
    case FTOUPPER:
    case FTOLOWER:
        buf = tostring(getsval(x));
        if (t == FTOUPPER) {
            for (p = buf; *p; p++)
                if (islower(*p))
                    *p = toupper(*p);
        } else {
            for (p = buf; *p; p++)
                if (isupper(*p))
                    *p = tolower(*p);
        }
        tempfree(x);
        x = gettemp();
        setsval(x, buf);
        free(buf);
        return x;
    case FFLUSH:
        if (isrec(x) || strlen(getsval(x)) == 0) {
            flush_all();    /* fflush() or fflush("") -> all */
            u = 0;
        } else if ((fp = openfile(FFLUSH, getsval(x))) == NULL)
            u = EOF;
        else
            u = fflush(fp);
        break;
    case FUTF:
        wc = (int)getfval(x);
        mbc[wctomb(mbc, wc)] = 0;
        tempfree(x);
        x = gettemp();
        setsval(x, mbc);
        return x;
    default:    /* can't happen */
        FATAL("illegal function type %d", t);
        break;
    }
    tempfree(x);
    x = gettemp();
    setfval(x, u);
    if (nextarg != 0) {
        WARNING("warning: function has too many arguments");
        for ( ; nextarg; nextarg = nextarg->nnext)
            execute(nextarg);
    }
    return(x);
}

Cell *printstat(Node **a, int n)    /* print a[0] */
{
    int r;
    Node *x;
    Cell *y;
    FILE *fp;

    if (a[1] == 0)  /* a[1] is redirection operator, a[2] is file */
        fp = stdout;
    else
        fp = redirect(ptoi(a[1]), a[2]);
    for (x = a[0]; x != NULL; x = x->nnext) {
        y = execute(x);
        fputs(getsval(y), fp);
        tempfree(y);
        if (x->nnext == NULL)
            r = fputs(*ORS, fp);
        else
            r = fputs(*OFS, fp);
        if (r == EOF)
            FATAL("write error on %s", filename(fp));
    }
    if (a[1] != 0)
        if (fflush(fp) == EOF)
            FATAL("write error on %s", filename(fp));
    return(True);
}

Cell *nullproc(Node **a, int n)
{
    n = n;
    a = a;
    return 0;
}


FILE *redirect(int a, Node *b)  /* set up all i/o redirections */
{
    FILE *fp;
    Cell *x;
    char *fname;

    x = execute(b);
    fname = getsval(x);
    fp = openfile(a, fname);
    if (fp == NULL)
        FATAL("can't open file %s", fname);
    tempfree(x);
    return fp;
}

struct files {
    FILE    *fp;
    char    *fname;
    int mode;   /* '|', 'a', 'w' => LE/LT, GT */
} files[FOPEN_MAX] ={
    { NULL,  "/dev/stdin",  LT },   /* watch out: don't free this! */
    { NULL, "/dev/stdout", GT },
    { NULL, "/dev/stderr", GT }
};

void stdinit(void)  /* in case stdin, etc., are not constants */
{
    files[0].fp = stdin;
    files[1].fp = stdout;
    files[2].fp = stderr;
}

FILE *openfile(int a, char *us)
{
    char *s = us;
    int i, m;
    FILE *fp = 0;

    if (*s == '\0')
        FATAL("null file name in print or getline");
    for (i=0; i < FOPEN_MAX; i++)
        if (files[i].fname && strcmp(s, files[i].fname) == 0) {
            if (a == files[i].mode || (a==APPEND && files[i].mode==GT))
                return files[i].fp;
            if (a == FFLUSH)
                return files[i].fp;
        }
    if (a == FFLUSH)    /* didn't find it, so don't create it! */
        return NULL;

    for (i=0; i < FOPEN_MAX; i++)
        if (files[i].fp == 0)
            break;
    if (i >= FOPEN_MAX)
        FATAL("%s makes too many open files", s);
    fflush(stdout); /* force a semblance of order */
    m = a;
    if (a == GT) {
        fp = fopen(s, "w");
    } else if (a == APPEND) {
        fp = fopen(s, "a");
        m = GT; /* so can mix > and >> */
    } else if (a == '|') {  /* output pipe */
        fp = popen(s, "w");
    } else if (a == LE) {   /* input pipe */
        fp = popen(s, "r");
    } else if (a == LT) {   /* getline <file */
        fp = strcmp(s, "-") == 0 ? stdin : fopen(s, "r");   /* "-" is stdin */
    } else  /* can't happen */
        FATAL("illegal redirection %d", a);
    if (fp != NULL) {
        files[i].fname = tostring(s);
        files[i].fp = fp;
        files[i].mode = m;
    }
    return fp;
}

char *filename(FILE *fp)
{
    int i;

    for (i = 0; i < FOPEN_MAX; i++)
        if (fp == files[i].fp)
            return files[i].fname;
    return "???";
}

Cell *closefile(Node **a, int n)
{
    Cell *x;
    int i, stat;

    n = n;
    x = execute(a[0]);
    getsval(x);
    for (i = 0; i < FOPEN_MAX; i++)
        if (files[i].fname && strcmp(x->sval, files[i].fname) == 0) {
            if (ferror(files[i].fp))
                WARNING( "i/o error occurred on %s", files[i].fname );
            if (files[i].mode == '|' || files[i].mode == LE)
                stat = pclose(files[i].fp);
            else
                stat = fclose(files[i].fp);
            if (stat == EOF)
                WARNING( "i/o error occurred closing %s", files[i].fname );
            if (i > 2)  /* don't do /dev/std... */
                xfree(files[i].fname);
            files[i].fname = NULL;  /* watch out for ref thru this */
            files[i].fp = NULL;
        }
    tempfree(x);
    return(True);
}

void closeall(void)
{
    int i, stat;

    for (i = 0; i < FOPEN_MAX; i++)
        if (files[i].fp) {
            if (ferror(files[i].fp))
                WARNING( "i/o error occurred on %s", files[i].fname );
            if (files[i].mode == '|' || files[i].mode == LE)
                stat = pclose(files[i].fp);
            else
                stat = fclose(files[i].fp);
            if (stat == EOF)
                WARNING( "i/o error occurred while closing %s", files[i].fname );
        }
}

void flush_all(void)
{
    int i;

    for (i = 0; i < FOPEN_MAX; i++)
        if (files[i].fp)
            fflush(files[i].fp);
}

void backsub(char **pb_ptr, char **sptr_ptr);

Cell *sub(Node **a, int nnn)    /* substitute command */
{
    char *sptr, *pb, *q;
    Cell *x, *y, *result;
    char *t, *buf;
    void *p;
    int bufsz = recsize;

    if ((buf = (char *) malloc(bufsz)) == NULL)
        FATAL("out of memory in sub");
    x = execute(a[3]);  /* target string */
    t = getsval(x);
    if (a[0] == 0)      /* 0 => a[1] is already-compiled regexpr */
        p = (void *) a[1];  /* regular expression */
    else {
        y = execute(a[1]);
        p = compre(getsval(y));
        tempfree(y);
    }
    y = execute(a[2]);  /* replacement string */
    result = False;
    if (pmatch(p, t, t)) {
        sptr = t;
        adjbuf(&buf, &bufsz, 1+patbeg-sptr, recsize, 0, "sub");
        pb = buf;
        while (sptr < patbeg)
            *pb++ = *sptr++;
        sptr = getsval(y);
        while (*sptr != 0) {
            adjbuf(&buf, &bufsz, 5+pb-buf, recsize, &pb, "sub");
            if (*sptr == '\\') {
                backsub(&pb, &sptr);
            } else if (*sptr == '&') {
                sptr++;
                adjbuf(&buf, &bufsz, 1+patlen+pb-buf, recsize, &pb, "sub");
                for (q = patbeg; q < patbeg+patlen; )
                    *pb++ = *q++;
            } else
                *pb++ = *sptr++;
        }
        *pb = '\0';
        if (pb > buf + bufsz)
            FATAL("sub result1 %.30s too big; can't happen", buf);
        sptr = patbeg + patlen;
        if ((patlen == 0 && *patbeg) || (patlen && *(sptr-1))) {
            adjbuf(&buf, &bufsz, 1+strlen(sptr)+pb-buf, 0, &pb, "sub");
            while ((*pb++ = *sptr++) != 0)
                ;
        }
        if (pb > buf + bufsz)
            FATAL("sub result2 %.30s too big; can't happen", buf);
        setsval(x, buf);    /* BUG: should be able to avoid copy */
        result = True;;
    }
    tempfree(x);
    tempfree(y);
    free(buf);
    return result;
}

Cell *gsub(Node **a, int nnn)   /* global substitute */
{
    Cell *x, *y;
    char *rptr, *sptr, *t, *pb, *c;
    char *buf;
    void *p;
    int mflag, num;
    int bufsz = recsize;

    if ((buf = (char *)malloc(bufsz)) == NULL)
        FATAL("out of memory in gsub");
    mflag = 0;  /* if mflag == 0, can replace empty string */
    num = 0;
    x = execute(a[3]);  /* target string */
    c = t = getsval(x);
    if (a[0] == 0)      /* 0 => a[1] is already-compiled regexpr */
        p = (void *) a[1];  /* regular expression */
    else {
        y = execute(a[1]);
        p = compre(getsval(y));
        tempfree(y);
    }
    y = execute(a[2]);  /* replacement string */
    if (pmatch(p, t, c)) {
        pb = buf;
        rptr = getsval(y);
        do {
            if (patlen == 0 && *patbeg != 0) {  /* matched empty string */
                if (mflag == 0) {   /* can replace empty */
                    num++;
                    sptr = rptr;
                    while (*sptr != 0) {
                        adjbuf(&buf, &bufsz, 5+pb-buf, recsize, &pb, "gsub");
                        if (*sptr == '\\') {
                            backsub(&pb, &sptr);
                        } else if (*sptr == '&') {
                            char *q;
                            sptr++;
                            adjbuf(&buf, &bufsz, 1+patlen+pb-buf, recsize, &pb, "gsub");
                            for (q = patbeg; q < patbeg+patlen; )
                                *pb++ = *q++;
                        } else
                            *pb++ = *sptr++;
                    }
                }
                if (*c == 0)    /* at end */
                    goto done;
                adjbuf(&buf, &bufsz, 2+pb-buf, recsize, &pb, "gsub");
                *pb++ = *c++;
                if (pb > buf + bufsz)   /* BUG: not sure of this test */
                    FATAL("gsub result0 %.30s too big; can't happen", buf);
                mflag = 0;
            }
            else {  /* matched nonempty string */
                num++;
                sptr = c;
                adjbuf(&buf, &bufsz, 1+(patbeg-sptr)+pb-buf, recsize, &pb, "gsub");
                while (sptr < patbeg)
                    *pb++ = *sptr++;
                sptr = rptr;
                while (*sptr != 0) {
                    adjbuf(&buf, &bufsz, 5+pb-buf, recsize, &pb, "gsub");
                    if (*sptr == '\\') {
                        backsub(&pb, &sptr);
                    } else if (*sptr == '&') {
                        char *q;
                        sptr++;
                        adjbuf(&buf, &bufsz, 1+patlen+pb-buf, recsize, &pb, "gsub");
                        for (q = patbeg; q < patbeg+patlen; )
                            *pb++ = *q++;
                    } else
                        *pb++ = *sptr++;
                }
                c = patbeg + patlen;
                if ((c[-1] == 0) || (*c == 0))
                    goto done;
                if (pb > buf + bufsz)
                    FATAL("gsub result1 %.30s too big; can't happen", buf);
                mflag = 1;
            }
        } while (pmatch(p, t, c));
        sptr = c;
        adjbuf(&buf, &bufsz, 1+strlen(sptr)+pb-buf, 0, &pb, "gsub");
        while ((*pb++ = *sptr++) != 0)
            ;
    done:   if (pb > buf + bufsz)
            FATAL("gsub result2 %.30s too big; can't happen", buf);
        *pb = '\0';
        setsval(x, buf);    /* BUG: should be able to avoid copy + free */
    }
    tempfree(x);
    tempfree(y);
    x = gettemp();
    x->tval = NUM;
    x->fval = num;
    free(buf);
    return(x);
}

void backsub(char **pb_ptr, char **sptr_ptr)    /* handle \\& variations */
{                       /* sptr[0] == '\\' */
    char *pb = *pb_ptr, *sptr = *sptr_ptr;

    if (sptr[1] == '\\') {
        if (sptr[2] == '\\' && sptr[3] == '&') { /* \\\& -> \& */
            *pb++ = '\\';
            *pb++ = '&';
            sptr += 4;
        } else if (sptr[2] == '&') {    /* \\& -> \ + matched */
            *pb++ = '\\';
            sptr += 2;
        } else {            /* \\x -> \\x */
            *pb++ = *sptr++;
            *pb++ = *sptr++;
        }
    } else if (sptr[1] == '&') {    /* literal & */
        sptr++;
        *pb++ = *sptr++;
    } else              /* literal \ */
        *pb++ = *sptr++;

    *pb_ptr = pb;
    *sptr_ptr = sptr;
}
/*e: awk/run.c */
