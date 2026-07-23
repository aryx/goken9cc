/*
 * el/obj.c -- read one ea object file (mirrors linkers/5l/obj.c's
 * ldobj(), simplified: no archives, no versioning, no auto/param
 * tracking, no peephole rewrites -- see l.h for what's deliberately
 * left out and why).
 */
#include "l.h"

/*
 * claude: real arches' float.c (e.g. linkers/5l/float.c) decode Ieee
 * bit-by-bit for portability across whatever host runs the linker.
 * el only ever runs on the ordinary little-endian IEEE754 hosts this
 * project already targets natively, so a plain union reinterpret is
 * safe and far simpler than reimplementing that portable decode.
 */
static double
ieeedtod(Ieee *e)
{
    union { double d; struct { int32 l, h; } i; } u;
    u.i.l = e->l;
    u.i.h = e->h;
    return u.d;
}

Sym*	hash[NHASH];
Text*	firsttext;
Text*	lasttext;
Text*	curtext;
Import*	imports;
DataReloc*	datarelocs;
int	nerrors;

Sym*
lookup(char *name)
{
    Sym *s;
    char *p;
    ulong h;

    h = 0;
    for(p = name; *p; p++)
        h = h*37 + *p;
    h %= NHASH;
    for(s = hash[h]; s != S; s = s->link)
        if(strcmp(s->name, name) == 0)
            return s;
    s = malloc(sizeof(Sym));
    memset(s, 0, sizeof(Sym));
    s->name = strdup(name);
    s->link = hash[h];
    hash[h] = s;
    return s;
}

Text*
newtext(Sym *sym)
{
    Text *t;

    t = malloc(sizeof(Text));
    memset(t, 0, sizeof(Text));
    t->sym = sym;
    if(firsttext == nil)
        firsttext = t;
    else
        lasttext->link = t;
    lasttext = t;
    return t;
}

Instr*
newinstr(int as, Adr *from, Adr *to)
{
    Instr *i;

    i = malloc(sizeof(Instr));
    memset(i, 0, sizeof(Instr));
    i->as = as;
    i->from = *from;
    i->to = *to;
    if(curtext->first == nil)
        curtext->first = i;
    else
        curtext->last->link = i;
    curtext->last = i;
    return i;
}

void
addimport(char *symname, char *module, char *field)
{
    Import *im;

    im = malloc(sizeof(Import));
    im->symname = strdup(symname);
    im->module = strdup(module);
    im->field = strdup(field);
    im->link = imports;
    imports = im;
}

/*
 * growable byte buffer backing a data symbol, patched by ADATA/GLOBL
 * records -- `need` is the *logical* size the caller wants guaranteed
 * (a byte offset actually written, or GLOBL's declared size), which
 * becomes the new datasize (never shrunk, so a DATA before a GLOBL or
 * a GLOBL after a DATA both work); datacap is the buffer's own
 * allocation, grown geometrically and otherwise unrelated to it.
 */
static void
growdata(Sym *s, long need)
{
    long newcap;
    char *nd;

    if(need > s->datasize)
        s->datasize = need;
    if(need <= s->datacap)
        return;
    newcap = s->datacap? s->datacap*2 : 64;
    if(newcap < need)
        newcap = need;
    nd = malloc(newcap);
    memset(nd, 0, newcap);
    if(s->data)
        memmove(nd, s->data, s->datacap);
    free(s->data);
    s->data = nd;
    s->datacap = newcap;
}

/* one operand descriptor: type, unused-reg byte, symidx, name, payload */
static byte*
inopd(byte *p, Adr *a, Sym **h)
{
    int symidx;
    long l;
    int i;

    a->type = p[0];
    /* p[1] is the outopd()'s NOREG placeholder byte -- unused here */
    symidx = p[2];
    if(symidx < 0 || symidx >= NSYM) {
        diag("sym index out of range: %d", symidx);
        symidx = 0;
    }
    a->sym = h[symidx];
    a->name = p[3];
    p += 4;

    switch(a->type) {
    case D_NONE:
    case D_LOCAL:
    case D_GLOBAL:
    case D_BRANCH:
    case D_OREG:
    case D_CONST:
        l = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
        a->offset = l;
        p += 4;
        break;

    case D_VCONST:
        a->vval = 0;
        for(i = 7; i >= 0; i--)
            a->vval = (a->vval << 8) | p[i];
        p += 8;
        break;

    case D_SCONST:
        memmove(a->sval, p, NSNAME);
        p += NSNAME;
        break;

    case D_FCONST:
        {
            Ieee e;
            e.l = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
            e.h = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
            a->dval = ieeedtod(&e);
        }
        p += 8;
        break;

    default:
        diag("unknown operand type %d", a->type);
        errorexit();
    }
    return p;
}

void
readobj(char *file)
{
    Biobuf *bp;
    byte *buf, *p, *end;
    long size;
    Sym *h[NSYM];
    Sym *s;
    int symidx, name;
    byte *stop;
    Instr *ip;

    bp = Bopen(file, OREAD);
    if(bp == nil) {
        diag("cannot open %s", file);
        errorexit();
    }
    Bseek(bp, 0, 2);
    size = Boffset(bp);
    Bseek(bp, 0, 0);
    buf = malloc(size);
    if(Bread(bp, buf, size) != size) {
        diag("short read on %s", file);
        errorexit();
    }
    Bterm(bp);

    memset(h, 0, sizeof(h));
    p = buf;
    end = buf + size;

    while(p < end) {
        int op = p[0];

        if(op <= AXXX || op >= ALAST) {
            diag("%s: opcode out of range %d -- not an .e file?", file, op);
            errorexit();
        }

        if(op == ANAME) {
            name = p[1];
            symidx = p[2];
            stop = memchr(p+3, '\0', end-(p+3));
            if(stop == nil) {
                diag("%s: truncated ANAME", file);
                errorexit();
            }
            s = lookup((char*)p+3);
            if(symidx >= 0 && symidx < NSYM)
                h[symidx] = s;
            p = stop+1;
            continue;
        }

        {
            Adr from, to;
            int reg;
            long line;

            /*
             * claude: outcode() (assemblers/ea/obj.c) writes
             * [opcode][reg][line:4][from][to] -- reg carries a third
             * small-integer argument some pseudo-ops need (DATA's
             * width, TEXT/GLOBL's optional flags), exactly the role
             * outcode()'s `reg`/`con` parameter plays on every other
             * arch (e.g. ia/a.y's `LDATA name '/' con ',' imm` passes
             * the width as outcode's 3rd arg). It is not part of
             * either operand.
             */
            reg = p[1];
            line = p[2] | (p[3]<<8) | (p[4]<<16) | (p[5]<<24);
            USED(line);
            p += 6;
            p = inopd(p, &from, h);
            p = inopd(p, &to, h);

            switch(op) {
            case AEND:
                /*
                 * claude: end-of-object-file marker (ea's main.c
                 * cclean() emits exactly one, always last) -- unlike
                 * AENDCTL (a real per-block/loop/if wasm instruction,
                 * left for the default case below), this is metadata
                 * about the stream, not an instruction, and v1 only
                 * ever reads a single object file, so it just means
                 * "stop".
                 */
                free(buf);
                return;

            case AHISTORY:
                /* claude: no debug info kept in v1 -- see l.h */
                break;

            case ATEXT:
                if(from.sym == S) {
                    diag("TEXT without a symbol");
                    errorexit();
                }
                if(from.sym->type != SNONE && from.sym->type != SXREF)
                    diag("redefinition: %s", from.sym->name);
                from.sym->type = STEXT;
                curtext = newtext(from.sym);
                curtext->framesize = to.offset;
                break;

            case AGLOBL:
                if(from.sym == S) {
                    diag("GLOBL without a symbol");
                    errorexit();
                }
                if(from.sym->type == SNONE || from.sym->type == SXREF)
                    from.sym->type = SBSS;
                growdata(from.sym, to.offset);
                break;

            case ADATA:
                if(from.sym == S) {
                    diag("DATA without a symbol");
                    errorexit();
                }
                if(from.sym->type == SNONE)
                    from.sym->type = SBSS;
                growdata(from.sym, from.offset + reg);
                if(to.type == D_SCONST) {
                    memmove(from.sym->data + from.offset, to.sval, reg);
                } else if(to.type == D_CONST && to.sym == S) {
                    long v = to.offset;
                    int i;
                    for(i = 0; i < reg; i++) {
                        from.sym->data[from.offset+i] = v & 0xff;
                        v >>= 8;
                    }
                } else if(to.type == D_CONST && to.sym != S) {
                    DataReloc *dr = malloc(sizeof(DataReloc));
                    dr->targetsym = from.sym;
                    dr->targetoff = from.offset;
                    dr->width = reg;
                    dr->refsym = to.sym;
                    dr->addend = to.offset;
                    dr->link = datarelocs;
                    datarelocs = dr;
                } else {
                    diag("DATA: unsupported value kind %d", to.type);
                }
                break;

            default:
                if(curtext == nil) {
                    diag("instruction outside of any TEXT");
                    errorexit();
                }
                ip = newinstr(op, &from, &to);
                USED(ip);
                break;
            }
        }
    }
}
