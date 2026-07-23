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
 * the one-operand MOV (const push, including a symbol's address),
 * CALL to an imported function, RET. NOT exercised by any test yet:
 * loads/stores (D_OREG operands), locals/globals, conversions, and
 * control flow (BLOCK/LOOP/IF/BR) -- implemented from the spec, in
 * the same spirit as adding an opcode to a real arch's optab.c before
 * every encoding has a test, but treat them as unverified.
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
        diag("SP/FP-relative addressing not implemented yet (no shadow stack in el v1)");
        errorexit();
        return 0;
    }
}

/* push the value described by `a` onto the stack */
static void
fetch(Bytebuf *bb, Adr *a, Optab *o)
{
    switch(a->type) {
    case D_CONST:
        if(a->sym != S) {
            bbput(bb, 0x41);	/* i32.const: an address is always i32 */
            bbsleb(bb, symaddr(a));
            break;
        }
        switch(o->constkind) {
        case 'w': bbput(bb, 0x41); bbsleb(bb, a->offset); break;
        case 'q': bbput(bb, 0x42); bbsleb(bb, a->offset); break;
        default:  diag("float MOV given a plain constant"); errorexit();
        }
        break;
    case D_VCONST:
        bbput(bb, 0x42);
        bbsleb(bb, a->vval);
        break;
    case D_FCONST:
        if(o->constkind == 'f') {
            float f = a->dval;
            bbput(bb, 0x43);
            bbwrite(bb, &f, 4);
        } else {
            double d = a->dval;
            bbput(bb, 0x44);
            bbwrite(bb, &d, 8);
        }
        break;
    case D_OREG:
        if(o->loadop < 0) {
            diag("conversion from memory not implemented yet in el v1");
            errorexit();
        }
        bbput(bb, 0x41);
        bbsleb(bb, symaddr(a));
        bbput(bb, o->loadop);
        bbuleb(bb, 0);	/* align hint */
        bbuleb(bb, 0);	/* offset: folded into the pushed address above */
        break;
    case D_LOCAL:
        bbput(bb, 0x20);
        bbuleb(bb, a->offset);
        break;
    case D_GLOBAL:
        bbput(bb, 0x23);
        bbuleb(bb, a->offset);
        break;
    default:
        diag("unsupported MOV source kind %d", a->type);
        errorexit();
    }
}

/* consume the top-of-stack value into destination `a` */
static void
store(Bytebuf *bb, Adr *a, Optab *o)
{
    switch(a->type) {
    case D_LOCAL:
        bbput(bb, 0x21);
        bbuleb(bb, a->offset);
        break;
    case D_GLOBAL:
        bbput(bb, 0x24);
        bbuleb(bb, a->offset);
        break;
    case D_OREG:
        if(o->storeop < 0) {
            diag("conversion into memory not implemented yet in el v1");
            errorexit();
        }
        bbput(bb, o->storeop);
        bbuleb(bb, 0);
        bbuleb(bb, 0);
        break;
    default:
        diag("unsupported MOV destination kind %d", a->type);
        errorexit();
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
        bbput(bb, o->op);
        bbuleb(bb, ip->to.offset);
        break;
    case OMOVE:
        if(ip->from.type == D_NONE)
            fetch(bb, &ip->to, o);		/* one-operand: push, leave on stack */
        else {
            fetch(bb, &ip->from, o);
            store(bb, &ip->to, o);
        }
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
     * claude: exactly two types -- type 0 (void->void) for every
     * locally-defined function, type 1 (4xi32 -> i32) for every
     * import (see l.h's Import comment: v1 only supports WASI
     * fd_write's shape). Once ec exists and functions have real
     * signatures, this needs a proper type table instead.
     */
    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, 2);
    bbput(&bb, 0x60); bbuleb(&bb, 0); bbuleb(&bb, 0);
    bbput(&bb, 0x60); bbuleb(&bb, 4);
    bbput(&bb, 0x7F); bbput(&bb, 0x7F); bbput(&bb, 0x7F); bbput(&bb, 0x7F);
    bbuleb(&bb, 1); bbput(&bb, 0x7F);
    emitsection(1, &bb);

    if(nimports > 0) {
        memset(&bb, 0, sizeof(bb));
        bbuleb(&bb, nimports);
        for(im = imports; im != nil; im = im->link) {
            bbname(&bb, im->module);
            bbname(&bb, im->field);
            bbput(&bb, 0x00);
            bbuleb(&bb, 1);
        }
        emitsection(2, &bb);
    }

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, ntexts);
    for(t = firsttext; t != nil; t = t->link)
        bbuleb(&bb, 0);
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
        Sym *startsym = lookup("_start");
        int nexp = 1;
        if(startsym->type != STEXT)
            startsym = nil;
        else
            nexp++;
        bbuleb(&bb, nexp);
        bbname(&bb, "memory");
        bbput(&bb, 0x02);
        bbuleb(&bb, 0);
        if(startsym != nil) {
            bbname(&bb, "_start");
            bbput(&bb, 0x00);
            bbuleb(&bb, resolvecall(startsym));
        }
    }
    emitsection(7, &bb);

    memset(&bb, 0, sizeof(bb));
    bbuleb(&bb, ntexts);
    for(t = firsttext; t != nil; t = t->link) {
        Bytebuf body;
        memset(&body, 0, sizeof(body));
        bbuleb(&body, 0);	/* claude: no local-decl vector yet; see header comment */
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
