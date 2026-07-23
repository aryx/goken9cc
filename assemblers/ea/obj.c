/*s: ea/obj.c */
#include "a.h"

/*
 * claude: modeled on assemblers/5a/obj.c, minus arm's scond/bcode[]
 * conditional-branch rewriting (wasm has no instruction predication).
 * The wire record shape is otherwise unchanged: opcode, reg, stmtline,
 * then two operand descriptors -- kept even though most wasm
 * instructions only ever fill one of the two (the other stays
 * nullgen/D_NONE), so this stays a straightforward adaptation of
 * every other arch's obj.c rather than a new format el has to learn.
 */

/*s: function [[zname]](wasm) */
void
zname(char *n, int name, int symidx)
{
    Bputc(&obuf, ANAME);
    Bputc(&obuf, name);	/* D_EXTERN or D_STATIC */
    Bputc(&obuf, symidx);
    while(*n) {
        Bputc(&obuf, *n);
        n++;
    }
    Bputc(&obuf, '\0');
}
/*e: function [[zname]](wasm) */

/*s: function [[outopd]](wasm) */
void
outopd(Gen *a, int symidx)
{
    long l;
    vlong v;
    char *n;
    Ieee e;
    int i;

    Bputc(&obuf, a->type);
    Bputc(&obuf, NOREG);
    // idx in symbol table, 0 if no symbol involved in the operand
    Bputc(&obuf, symidx);
    // name of the symbol (D_EXTERN/D_STATIC), or D_NONE
    Bputc(&obuf, a->name);

    switch(a->type) {
    case D_NONE:
    case D_LOCAL:
    case D_GLOBAL:
        goto offcase;

    case D_BRANCH:
        /*
         * claude: a label depth, resolved entirely within a.y by
         * tracking the block/loop/if nesting as it's parsed -- unlike
         * every other arch's D_BRANCH, this never needs a second pass
         * over the whole program, because it's a lexical property of
         * the enclosing function alone. By the time outcode() is
         * called, $$.offset already *is* the depth; there is nothing
         * left for el to resolve.
         */
    case D_OREG:
    case D_CONST:
    offcase:
        l = a->offset;
        Bputc(&obuf, l);
        Bputc(&obuf, l>>8);
        Bputc(&obuf, l>>16);
        Bputc(&obuf, l>>24);
        break;

    case D_VCONST:
        v = a->vval;
        for(i = 0; i < 8; i++) {
            Bputc(&obuf, v);
            v >>= 8;
        }
        break;

    case D_SCONST:
        n = a->sval;
        for(i=0; i<NSNAME; i++) {
            Bputc(&obuf, *n);
            n++;
        }
        break;

    case D_FCONST:
        ieeedtod(&e, a->dval);
        Bputc(&obuf, e.l);
        Bputc(&obuf, e.l>>8);
        Bputc(&obuf, e.l>>16);
        Bputc(&obuf, e.l>>24);
        Bputc(&obuf, e.h);
        Bputc(&obuf, e.h>>8);
        Bputc(&obuf, e.h>>16);
        Bputc(&obuf, e.h>>24);
        break;

    default:
        print("unknown type %d\n", a->type);
        exits("arg");
    }
}
/*e: function [[outopd]](wasm) */

/*s: function [[symidx_of_symopt]](wasm) */
int
symidx_of_symopt(Sym *sym, int name)
{
    int idx = 0;

    if(sym != S) {
        idx = sym->symidx;
        if(idx < 0 || idx >= NSYM)
            idx = 0;

        // already generated an ANAME for this symbol reference?
        if(h[idx].sym != sym || h[idx].symkind != name) {
            sym->symidx = symcounter;
            h[symcounter].sym = sym;
            h[symcounter].symkind = name;
            idx = symcounter;
            zname(sym->name, name, symcounter);

            symcounter++;
            if(symcounter >= NSYM)
                symcounter = 1;	/* circular array */
        }
    }
    return idx;
}
/*e: function [[symidx_of_symopt]](wasm) */

/*s: function [[outcode]](wasm) */
void
outcode(int opcode, Gen *g1, int reg, Gen *g2)
{
    int sf, st, oldsymcounter;

    if(pass == 1)
        goto out;

    jackpot:
    oldsymcounter = symcounter;
    sf = symidx_of_symopt(g1->sym, g1->name);
    st = symidx_of_symopt(g2->sym, g2->name);
    if(sf == st && sf != 0 && symcounter != oldsymcounter)
        goto jackpot;

    Bputc(&obuf, opcode);
    Bputc(&obuf, reg);
    Bputc(&obuf, stmtline);
    Bputc(&obuf, stmtline>>8);
    Bputc(&obuf, stmtline>>16);
    Bputc(&obuf, stmtline>>24);
    outopd(g1, sf);
    outopd(g2, st);

out:
    if(opcode != AGLOBL && opcode != ADATA)
        pc++;
}
/*e: function [[outcode]](wasm) */

/*s: function [[outhist]](wasm) */
void
outhist(void)
{
    Gen g;
    Hist *h;
    char *p, *q, *op;
    int n;

    g = nullgen;
    for(h = hist; h != H; h = h->link) {
        p = h->filename;

        if(!debug['r'] && p && p[0] != '/' && h->local_line == 0 && pathname && pathname[0] == '/') {
            op = p;
            p = pathname;
        } else {
            op = nil;
        }
        while(p) {
            q = strchr(p, '/');
            if(q) {
                n = q-p;
                if(n == 0){
                    n = 1;	/* leading "/" */
                    *p = '/';
                }
                q++;
            } else {
                n = strlen(p);
                q = nil;
            }

            if(n) {
                Bputc(&obuf, ANAME);
                Bputc(&obuf, D_FILE);
                Bputc(&obuf, 1);
                Bputc(&obuf, '<');
                Bwrite(&obuf, p, n);
                Bputc(&obuf, '\0');
            }
            p = q;
            if(p == nil && op) {
                p = op;
                op = nil;
            }
        }

        Bputc(&obuf, AHISTORY);
        Bputc(&obuf, NOREG);
        Bputc(&obuf, h->global_line);
        Bputc(&obuf, h->global_line>>8);
        Bputc(&obuf, h->global_line>>16);
        Bputc(&obuf, h->global_line>>24);
        outopd(&nullgen, 0);
        g.offset = h->local_line;
        outopd(&g, 0);
    }
}
/*e: function [[outhist]](wasm) */
/*e: ea/obj.c */
