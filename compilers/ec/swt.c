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
 * claude: a scratch CAUTO-shaped Node with a freshly reserved,
 * non-colliding local slot -- ported from ic/txt.c's own regsalloc(),
 * same cursafe/curarg/maxargsafe/stkoff globals (already declared for
 * every backend via cc.h/gc.h) and the same align()-based offset
 * computation, just against ec's own align() (whose Aaut3 case hands
 * out a flat +1 per call rather than a byte width, see align()'s own
 * comment -- so this reserves one wasm local per call, exactly the
 * granularity swit1() below needs). codgen() (cck/pgen.c) already
 * folds maxargsafe into the ATEXT frame size *after* gen() finishes
 * (`sp->to.offset += maxargsafe`), which is what lets this be called
 * from deep inside statement codegen despite ATEXT already having
 * been emitted with the "final" auto count by then -- the real
 * mechanism ic/6c already rely on for exactly this kind of need
 * (their own regsalloc() calls, e.g. ic/cgen.c's indirect-call spill).
 * Unlike a real local, xoffset here is computed directly rather than
 * assigned by dcl.c's declaration-time walk, but naddr()/localindex()
 * don't know or care about that difference -- to them this is just
 * another CAUTO.
 */
void
regsalloc(Node *n, Node *nn)
{
	cursafe = align(cursafe + stkoff, nn->type, Aaut3) - stkoff;
	maxargsafe = maxround(maxargsafe, cursafe + curarg);
	*n = *nodsafe;
	n->xoffset = -(stkoff + cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
}

/*
 * claude: doswit()/casf()/nullwarn()/ieeedtod() are already defined in
 * the shared compilers/cck/pswt.c -- only swit1() (the actual
 * compare-and-branch emission, genuinely arch-specific) is ec's to
 * provide.
 *
 * By the time this runs, doswit() has already cgen()'d the switch
 * expression into `n` -- meaning its value is sitting on the wasm
 * operand stack *right now*. But swit1() needs to compare that one
 * value against up to nc different case constants, and a wasm
 * operand-stack value can only be consumed once (no dup in wasm MVP,
 * same limitation as everywhere else in this backend) -- so the very
 * first thing here is to pop it into a fresh scratch local
 * (regsalloc() above), which -- unlike the operand stack -- can be
 * read back as many times as needed via ALOCALGET.
 *
 * Each case becomes "push scratch; push the case constant; i32.eq;
 * br_if to that case's label" -- q[i].label is already a concrete,
 * resolved pc by this point (the case body was emitted earlier, by
 * doswit()'s own gen(n->right) call, before the switch expression was
 * even evaluated), so patch() can target it immediately instead of
 * being deferred the way an ordinary if/while/for's own gbranch()
 * placeholders are. Falls through to an unconditional branch to `def`
 * (the default case's label, or breakpc if there was no default --
 * doswit() already resolved that distinction before calling this).
 *
 * Int-only for this bootstrap (matches every other switch-adjacent
 * piece of this backend): doswit() only ever reaches the `!isv`
 * (32-bit) call shape for ec, since typev[] is never true for the
 * char/int switch expressions the shared sources use.
 */
void
swit1(C1 *q, int nc, int32 def, Node *n)
{
	Node nsafe;
	int i;

	regsalloc(&nsafe, n);
	gins(ALOCALSET, Z, &nsafe);

	for(i = 0; i < nc; i++) {
		gins(ALOCALGET, Z, &nsafe);
		gins(ACONSTW, Z, nodconst((int32)q[i].val));
		gins(ACMPEQW, Z, Z);
		nextpc();
		p->as = ABRIF;
		p->height = stackheight;
		stackheight--;	/* pops its condition -- see txt.c's stackdelta() comment */
		patch(p, q[i].label);
	}
	gbranch(OGOTO);
	patch(p, def);
}

/*
 * claude: ported from ic/swt.c almost verbatim (see docs/notes_wasm.txt) --
 * buffers up to NSNAME bytes at a time into the shared `.string` blob
 * (symstring, wired up once in cck/lex.c), flushing each full chunk as
 * an ADATA record. Uses p->reg to carry the chunk's byte count, not
 * p->from.scale (6c's convention): ec/e.out.h's Adr has no scale
 * field, and el/obj.c's ADATA reader already expects the count in
 * `reg` (outcode()'s generic 3rd-argument slot -- see its own
 * comment), matching every other ADATA-emitting backend that doesn't
 * have a scale field either (ic among them).
 */
long
outstring(char *s, long n)
{
	long r;

	if(suppress)
		return nstring;
	r = nstring;
	while(n) {
		string[mnstring] = *s++;
		mnstring++;
		nstring++;
		if(mnstring >= NSNAME) {
			gpseudo(ADATA, symstring, nodconst(0L));
			p->from.offset += nstring - NSNAME;
			p->reg = NSNAME;
			p->to.type = D_SCONST;
			memmove(p->to.sval, string, NSNAME);
			mnstring = 0;
		}
		n--;
	}
	return r;
}
