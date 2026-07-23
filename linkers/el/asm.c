/*
 * el/asm.c -- emit a real .wasm binary module.
 *
 * claude: plays the role of every other arch's asm.c (final code
 * emission) plus layout.c/span.c combined (assigning addresses/
 * indices) -- there's no separate pass needed here to iterate
 * addresses to a fixpoint the way span.c's branch-shortening does on
 * a real arch, because wasm branches are structured/depth-based (see
 * assemblers/ea/a.y) and already fully resolved by the time el reads
 * them. So layout is a single pass: assign every symbol a function
 * index or arena offset, then emit -- each instruction's actual
 * encoding comes from optab.c's oplook(), not from logic in here.
 *
 * Verified against tests/s/mini/hello_wasm.s: TEXT/GLOBL/DATA layout,
 * ACONSTW (including pushing a symbol's address), CALL to an imported
 * function, RET, ADROP. NOT exercised by any test yet: loads/stores,
 * locals/globals, conversions, and control flow (BLOCK/LOOP/IF/BR) --
 * implemented from the spec, in the same spirit as adding an opcode to
 * a real arch's optab.c before every encoding has a test, but treat
 * them as unverified.
 */
#include "l.h"

#define	ARENABASE	8	/* leave a small null-guard region unused */

static long arenaend;
static Sym *allsyms;	/* insertion order for SBSS layout, see collectsyms() */

/*
 * claude: bio has no growable in-memory Biobuf, so each section body
 * is built into a plain malloc'd byte buffer via this tiny growable-
 * append helper, then copied into obuf with its id+size prefix by
 * emitsection() -- the wasm-binary counterpart of how a real arch's
 * asm.c buffers a segment before writing it with its header.
 */
typedef struct { char *buf; long len, cap; } Bytebuf;

static void
bbput(Bytebuf *bb, int c)
{
    if(bb->len >= bb->cap) {
        bb->cap = bb->cap? bb->cap*2 : 64;
        bb->buf = realloc(bb->buf, bb->cap);
    }
    bb->buf[bb->len++] = c;
}

static void
bbwrite(Bytebuf *bb, void *p, long n)
{
    long i;
    for(i = 0; i < n; i++)
        bbput(bb, ((char*)p)[i]);
}

static void
bbuleb(Bytebuf *bb, uvlong v)
{
    for(;;) {
        int byte = v & 0x7f;
        v >>= 7;
        if(v != 0)
            bbput(bb, byte | 0x80);
        else {
            bbput(bb, byte);
            break;
        }
    }
}

static void
bbsleb(Bytebuf *bb, vlong v)
{
    int more = 1;
    while(more) {
        int byte = v & 0x7f;
        v >>= 7;	/* arithmetic shift: sign-extends */
        if((v == 0 && !(byte & 0x40)) || (v == -1 && (byte & 0x40)))
            more = 0;
        else
            byte |= 0x80;
        bbput(bb, byte);
    }
}

static void
bbname(Bytebuf *bb, char *s)
{
    bbuleb(bb, strlen(s));
    bbwrite(bb, s, strlen(s));
}

static void
emitsection(int id, Bytebuf *bb)
{
    Bputc(&obuf, id);
    /* uleb straight into obuf: sections are framed by id+size+body */
    {
        uvlong v = bb->len;
        for(;;) {
            int byte = v & 0x7f;
            v >>= 7;
            if(v != 0)
                Bputc(&obuf, byte | 0x80);
            else {
                Bputc(&obuf, byte);
                break;
            }
        }
    }
    Bwrite(&obuf, bb->buf, bb->len);
    free(bb->buf);
    memset(bb, 0, sizeof(*bb));
}

/* ---- function signatures (real per-function types, from ec's ASIGNATURE) ---- */

/*
 * claude: a signature string is exactly what l.h's Text.sig comment
 * describes: one value-type char per parameter, then the result type
 * or 'V' for void -- used directly (no separate parsed struct needed)
 * as the dedup key, since two Texts with the identical string need
 * the identical wasm type-section entry.
 */
#define	MAXSIG	64
static char sigtab[MAXSIG][NSNAME];
static int nsigs;

/* the one shape v1 needs for imports (WASI fd_write); see l.h's Import comment */
#define	IMPORTSIG	"WWWWW"

static int
valtype(int c)
{
    switch(c) {
    case 'W': return 0x7F;	/* i32 */
    case 'Q': return 0x7E;	/* i64 */
    case 'F': return 0x7D;	/* f32 */
    case 'D': return 0x7C;	/* f64 */
    }
    diag("bad signature type char '%c'", c);
    errorexit();
    return 0;
}

static int
sigindex(char *sig)
{
    int i;

    for(i = 0; i < nsigs; i++)
        if(strcmp(sigtab[i], sig) == 0)
            return i;
    if(nsigs >= MAXSIG) {
        diag("too many distinct function signatures (max %d)", MAXSIG);
        errorexit();
    }
    strncpy(sigtab[nsigs], sig, NSNAME);
    return nsigs++;
}

static void
emittype(Bytebuf *bb, char *sig)
{
    char *p;
    int nparam;

    nparam = strlen(sig) - 1;
    bbput(bb, 0x60);
    bbuleb(bb, nparam);
    for(p = sig; p < sig+nparam; p++)
        bbput(bb, valtype(*p));
    if(sig[nparam] == 'V') {
        bbuleb(bb, 0);
    } else {
        bbuleb(bb, 1);
        bbput(bb, valtype(sig[nparam]));
    }
}

/* ---- symbol resolution ---- */

/*
 * claude: lookup() (obj.c) doesn't track insertion order (hash bucket
 * order isn't it), so SBSS symbols are collected into `allsyms` here,
 * in whatever order hash[] happens to walk -- documented as not
 * (yet) giving the `-r`-style reproducibility real arches guarantee
 * (see 5a/obj.c's outhist); giving Sym its own insertion-order link
 * would fix it, left for when a second object file makes it matter.
 */
static void
collectsyms(void)
{
    int i;
    Sym *s;

    for(i = 0; i < NHASH; i++)
        for(s = hash[i]; s != S; s = s->link)
            if(s->type == SBSS) {
                s->link2 = allsyms;
                allsyms = s;
            }
}

static int
resolvecall(Sym *s)
{
    Import *im;
    Text *t;
    int idx;

    idx = 0;
    for(im = imports; im != nil; im = im->link)
        idx++;
    for(t = firsttext; t != nil; t = t->link, idx++)
        if(t->sym == s)
            return idx;
    idx = 0;
    for(im = imports; im != nil; im = im->link, idx++)
        if(strcmp(im->symname, s->name) == 0)
            return idx;
    diag("undefined symbol: %s (not TEXT'd and no -I import declares it)", s->name);
    errorexit();
    return -1;
}

static void
layout(void)
{
    Sym *s;
    long off;
    DataReloc *dr;

    collectsyms();

    off = ARENABASE;
    for(s = allsyms; s != nil; s = s->link2) {
        s->value = off;
        off += (s->datasize + 3) & ~3;	/* 4-byte align */
    }
    arenaend = off;

    for(dr = datarelocs; dr != nil; dr = dr->link) {
        long v;
        int i;

        if(dr->refsym->type != SBSS) {
            diag("DATA relocation against non-data symbol: %s", dr->refsym->name);
            errorexit();
        }
        v = dr->refsym->value + dr->addend;
        for(i = 0; i < dr->width; i++) {
            dr->targetsym->data[dr->targetoff+i] = v & 0xff;
            v >>= 8;
        }
    }
}

/* ---- per-instruction codegen, driven by optab.c ---- */

/*
 * claude: resolves the address ACONSTx's "push a symbol's address"
 * form needs (see e.out.h's D_OREG comment). Only ever reached for
 * D_EXTERN/D_STATIC in practice: ea's ALOADx/ASTOREx expansion (a.y's
 * pushaddr()) handles D_AUTO/D_PARAM itself, by emitting an
 * AGLOBALGET(SPGLOBAL) instead of ever asking el to fold a
 * stack-relative address into a constant -- which it couldn't, since
 * the shadow stack's base is only known at run time, not link time.
 */
static long
symaddr(Adr *a)
{
    switch(a->name) {
    case D_EXTERN:
    case D_STATIC:
        if(a->sym->type != SBSS) {
            diag("address-of a non-data symbol: %s", a->sym->name);
            errorexit();
        }
        return a->sym->value + a->offset;
    default:
        diag("SP/FP-relative constant address not supported (not a link-time constant)");
        errorexit();
        return 0;
    }
}

static void
emitinstr(Bytebuf *bb, Instr *ip)
{
    Optab *o = oplook(ip->as);

    switch(o->kind) {
    case OSIMPLE:
        bbput(bb, o->op);
        break;
    case OSIMPLE2:
        bbput(bb, o->op);
        bbput(bb, o->op2);
        break;
    case OBR:
        bbput(bb, o->op);
        bbuleb(bb, ip->to.offset);
        break;
    case OCALL:
        bbput(bb, o->op);
        bbuleb(bb, resolvecall(ip->to.sym));
        break;
    case OLOCAL:
    case OGLOBAL:
        bbput(bb, o->op);
        bbuleb(bb, ip->to.offset);
        break;
    case OCONSTI:
        bbput(bb, o->op);
        if(ip->to.type == D_VCONST)
            bbsleb(bb, ip->to.vval);
        else if(ip->to.sym != S)
            bbsleb(bb, symaddr(&ip->to));
        else
            bbsleb(bb, ip->to.offset);
        break;
    case OCONSTF:
        bbput(bb, o->op);
        if(o->fsize == 4) {
            float f = ip->to.dval;
            bbwrite(bb, &f, 4);
        } else {
            double d = ip->to.dval;
            bbwrite(bb, &d, 8);
        }
        break;
    case OMEM:
        bbput(bb, o->op);
        bbuleb(bb, 0);		/* align hint, always minimal */
        bbuleb(bb, ip->to.offset);	/* the memarg offset ea's pushaddr() computed */
        break;
    }
}

/* ---- top-level module ---- */

void
asmb(void)
{
    Bytebuf bb;
    Text *t;
    Instr *ip;
    Import *im;
    int nimports, ntexts;
    Sym *s;

    layout();

    nimports = 0;
    for(im = imports; im != nil; im = im->link)
        nimports++;
    ntexts = 0;
    for(t = firsttext; t != nil; t = t->link)
        ntexts++;

    Bwrite(&obuf, "\0asm", 4);
    Bputc(&obuf, 1); Bputc(&obuf, 0); Bputc(&obuf, 0); Bputc(&obuf, 0);

    /*
     * claude: a real, deduplicated type table -- one entry per
     * distinct signature actually used, built from ec's ASIGNATURE
     * records (l.h's Text.sig; 'V'/no-ASIGNATURE for a hand-written
     * .s TEXT, which takes no arguments and returns nothing a caller
     * could use, matching every hello_*.s's _start) plus the one
     * hardcoded import shape (see l.h's Import comment).
     */
    sigindex(IMPORTSIG);
    for(t = firsttext; t != nil; t = t->link)
        sigindex(t->sig);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, nsigs);
    {
        int i;
        for(i = 0; i < nsigs; i++)
            emittype(&bb, sigtab[i]);
    }
    emitsection(1, &bb);

    if(nimports > 0) {
        memset(&bb, 0, sizeof(bb));
        bbuleb(&bb, nimports);
        for(im = imports; im != nil; im = im->link) {
            bbname(&bb, im->module);
            bbname(&bb, im->field);
            bbput(&bb, 0x00);
            bbuleb(&bb, sigindex(IMPORTSIG));
        }
        emitsection(2, &bb);
    }

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, ntexts);
    for(t = firsttext; t != nil; t = t->link)
        bbuleb(&bb, sigindex(t->sig));
    emitsection(3, &bb);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, 1);
    bbput(&bb, 0x00);
    bbuleb(&bb, 1);
    emitsection(5, &bb);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, 1);
    bbput(&bb, 0x7F); bbput(&bb, 0x01);
    bbput(&bb, 0x41); bbsleb(&bb, arenaend);
    bbput(&bb, 0x0B);
    emitsection(6, &bb);

    memset(&bb, 0, sizeof(bb));
    {
        /*
         * claude: _start, same convention as every other arch's
         * hello_*.s here (and every real Plan9 program: the C runtime
         * itself is what calls main() from _start, not something a
         * linker special-cases per language).
         *
         * Every other Text function is exported too, under its own
         * name -- ec has no export-annotation mechanism yet, so
         * "export everything" is the simplest useful default while
         * bootstrapping (see docs/notes_wasm.txt): it's what lets a
         * host (or a test harness) call e.g. a single compiled
         * function directly, without needing a working _start/WASI
         * path at all yet.
         */
        Sym *startsym = lookup("_start");
        int idx;

        if(startsym->type != STEXT)
            startsym = nil;

        bbuleb(&bb, 1 + ntexts);	/* memory, plus one export per Text (_start included) */
        bbname(&bb, "memory");
        bbput(&bb, 0x02);
        bbuleb(&bb, 0);
        if(startsym != nil) {
            bbname(&bb, "_start");
            bbput(&bb, 0x00);
            bbuleb(&bb, resolvecall(startsym));
        }
        idx = nimports;
        for(t = firsttext; t != nil; t = t->link, idx++) {
            if(t->sym == startsym)
                continue;	/* already exported above */
            bbname(&bb, t->sym->name);
            bbput(&bb, 0x00);
            bbuleb(&bb, idx);
        }
    }
    emitsection(7, &bb);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, ntexts);
    for(t = firsttext; t != nil; t = t->link) {
        Bytebuf body;
        memset(&body, 0, sizeof(body));
        /*
         * claude: t->framesize is ec's own local (auto) count, not a
         * byte size -- see compilers/ec/txt.c's align()/Aaut3 comment.
         * Every auto is a plain i32 slot for now (int-only bootstrap,
         * same scope as ASIGNATURE's 'W'-only signature), so one
         * local-decl entry declaring that many i32s covers them all.
         */
        if(t->framesize > 0) {
            bbuleb(&body, 1);
            bbuleb(&body, t->framesize);
            bbput(&body, 0x7F);	/* i32 */
        } else {
            bbuleb(&body, 0);
        }
        for(ip = t->first; ip != nil; ip = ip->link)
            emitinstr(&body, ip);
        bbput(&body, 0x0B);	/* mandatory function-closing end */

        bbuleb(&bb, body.len);
        bbwrite(&bb, body.buf, body.len);
        free(body.buf);
    }
    emitsection(10, &bb);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, 1);
    bbput(&bb, 0x00);
    bbput(&bb, 0x41); bbsleb(&bb, ARENABASE);
    bbput(&bb, 0x0B);
    bbuleb(&bb, arenaend - ARENABASE);
    {
        char *arena = malloc(arenaend - ARENABASE);
        memset(arena, 0, arenaend - ARENABASE);
        for(s = allsyms; s != nil; s = s->link2)
            memmove(arena + (s->value - ARENABASE), s->data, s->datasize);
        bbwrite(&bb, arena, arenaend - ARENABASE);
        free(arena);
    }
    emitsection(11, &bb);

    Bflush(&obuf);
}
