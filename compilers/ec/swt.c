/*
 * ec/swt.c -- writes the .e object file directly, in the exact same
 * wire format assemblers/ea/obj.c's outcode()/outopd()/zname() write
 * (and el/obj.c already knows how to read) -- see docs/notes_wasm.txt
 * for why Xc bypasses Xa entirely instead of emitting textual .s for
 * ea to reprocess (confirmed against compilers/ic/swt.c, which does
 * the same thing for riscv).
 *
 * Simpler than ea's assemble(): ec's Prog list (firstp..lastp) is
 * already a complete, fully-resolved list by the time gclean() calls
 * outcode() here (every nextpc()/patch() call already happened while
 * walking the parsed C source), so this is a single linear pass, not
 * ea's two-pass "resolve forward references, then emit" scheme.
 */
#include "gc.h"

static Sym*	h[NSYM];
static int	symcounter = 1;

static void
zname(char *n, int name, int symidx)
{
	Bputc(&outbuf, ANAME);
	Bputc(&outbuf, name);
	Bputc(&outbuf, symidx);
	while(*n) {
		Bputc(&outbuf, *n);
		n++;
	}
	Bputc(&outbuf, '\0');
}

static int
symidx_of_symopt(Sym *sym, int name)
{
	int idx;

	idx = 0;
	if(sym != S) {
		idx = sym->sym;
		if(idx < 0 || idx >= NSYM)
			idx = 0;
		if(h[idx] != sym) {
			sym->sym = symcounter;
			h[symcounter] = sym;
			idx = symcounter;
			zname(sym->name, name, symcounter);
			symcounter++;
			if(symcounter >= NSYM)
				symcounter = 1;
		}
	}
	return idx;
}

static void
outopd(Adr *a, int symidx)
{
	long l;
	int i;
	Ieee e;

	Bputc(&outbuf, a->type);
	Bputc(&outbuf, 0);	/* claude: NOREG placeholder, matches ea/obj.c's wire shape */
	Bputc(&outbuf, symidx);
	Bputc(&outbuf, a->name);

	switch(a->type) {
	case D_NONE:
	case D_LOCAL:
	case D_GLOBAL:
	case D_BRANCH:
	case D_OREG:
	case D_CONST:
		l = a->offset;
		Bputc(&outbuf, l);
		Bputc(&outbuf, l>>8);
		Bputc(&outbuf, l>>16);
		Bputc(&outbuf, l>>24);
		break;

	case D_VCONST:
		{
			vlong v = a->vval;
			for(i = 0; i < 8; i++) {
				Bputc(&outbuf, v);
				v >>= 8;
			}
		}
		break;

	case D_SCONST:
		for(i = 0; i < NSNAME; i++)
			Bputc(&outbuf, a->sval[i]);
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		Bputc(&outbuf, e.l);
		Bputc(&outbuf, e.l>>8);
		Bputc(&outbuf, e.l>>16);
		Bputc(&outbuf, e.l>>24);
		Bputc(&outbuf, e.h);
		Bputc(&outbuf, e.h>>8);
		Bputc(&outbuf, e.h>>16);
		Bputc(&outbuf, e.h>>24);
		break;

	default:
		diag(Z, "unknown type %d in outopd", a->type);
	}
}

void
outcode(void)
{
	Prog *q;
	int sf, st;

	for(q = firstp; q != P; q = q->link) {
		sf = symidx_of_symopt(q->from.sym, q->from.name);
		st = symidx_of_symopt(q->to.sym, q->to.name);

		Bputc(&outbuf, q->as);
		Bputc(&outbuf, q->reg);
		Bputc(&outbuf, q->lineno);
		Bputc(&outbuf, q->lineno>>8);
		Bputc(&outbuf, q->lineno>>16);
		Bputc(&outbuf, q->lineno>>24);
		outopd(&q->from, sf);
		outopd(&q->to, st);
	}
}

void
gextern(Sym *s, Node *n, long off, long w)
{
	Node nod;

	USED(n);
	nod = *nodconst(w);
	if(off || s->class == CSTATIC) {
		/* claude: not implemented -- ec has no test with initialized
		 * globals yet (see docs/notes_wasm.txt); a plain zero-size
		 * GLOBL (below) is enough to reserve space. */
	}
	gpseudo(AGLOBL, s, &nod);
}

/*
 * claude: doswit()/casf()/nullwarn()/ieeedtod() are already defined
 * in the shared compilers/cck/pswt.c -- only swit1() (the actual
 * compare-and-branch emission, genuinely arch-specific) is ec's to
 * provide. Switch isn't implemented (see txt.c's gbranch()/patch()),
 * so this is a stub; it's never reached because gen()'s OSWITCH case
 * would already have called gbranch(OGOTO), which diag()s first.
 */
void
swit1(C1 *c1, int nc, int32 def, Node *n)
{
	USED(c1); USED(nc); USED(def); USED(n);
	diag(Z, "switch not implemented yet in ec");
}

long
outstring(char *s, long n)
{
	USED(s);
	return n;
}
