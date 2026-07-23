/*
 * ec/txt.c -- core codegen primitives.
 *
 * claude: modeled on compilers/ic/txt.c's shape, but drastically
 * simpler: there is no register file, so no naddr()/gmove() cross
 * table of register-vs-memory addressing modes, no regalloc()
 * bookkeeping, no per-argument register-vs-stack calling-convention
 * split (gargs() below just evaluates each argument in order onto
 * the wasm value stack -- exactly wasm's own calling convention, see
 * e.out.h's AMOVx-removal comment). See docs/notes_wasm.txt.
 */
#include "gc.h"

/* claude: forward declaration -- gins() (below) needs to reset this on
 * every ordinary instruction; its real home and full explanation are
 * with gbranch()/patch(), further down, where it's actually used. */
static int lastterm;

void
ginit(void)
{
	int i;

	thechar = 'e';
	thestring = "wasm";
	ewidth[TIND] = SZ_IND;

	listinit();
	nstring = 0;
	mnstring = 0;
	nrathole = 0;
	pc = 0;
	breakpc = -1;
	continpc = -1;
	cases = C;
	firstp = P;
	lastp = P;
	tfield = types[TLONG];

	zprog.link = P;
	zprog.as = AGOK;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.to = zprog.from;

	/*
	 * claude: the sentinel meaning "value is on the wasm operand
	 * stack" -- see gc.h's comment. Reuses OREGISTER purely because
	 * every other arch already uses that op to mean "this Node
	 * denotes a machine location the value lives in"; there being
	 * only one such location here, no reg number is stored at all.
	 */
	stackresult.op = OREGISTER;
	stackresult.class = CXXX;
	stackresult.complex = 0;
	stackresult.addable = 11;

	constnode.op = OCONST;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;
	constnode.type = types[TLONG];

	vconstnode = constnode;
	vconstnode.type = types[TVLONG];

	fconstnode.op = OCONST;
	fconstnode.class = CXXX;
	fconstnode.complex = 0;
	fconstnode.addable = 20;
	fconstnode.type = types[TDOUBLE];

	nodsafe = new(ONAME, Z, Z);
	nodsafe->sym = slookup(".safe");
	nodsafe->type = types[TINT];
	nodsafe->etype = types[TINT]->etype;
	nodsafe->class = CAUTO;
	complex(nodsafe);

	nodrat = Z;
	nodret = Z;

	com64init();

	nlocal = 0;
	USED(i);
}

void
gclean(void)
{
	int i;
	Sym *s;

	while(mnstring)
		outstring("", 1L);
	symstring->type->width = nstring;
	for(i = 0; i < NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == T)
			continue;
		if(s->type->width == 0)
			continue;
		if(s->class != CGLOBL && s->class != CSTATIC)
			continue;
		if(s->type == types[TENUM])
			continue;
		gpseudo(AGLOBL, s, nodconst(s->type->width));
	}
	nextpc();
	p->as = AEND;
	outcode();
}

void
nextpc(void)
{
	p = alloc(sizeof(*p));
	*p = zprog;
	p->lineno = nearln;
	pc++;
	if(firstp == P) {
		firstp = p;
		lastp = p;
		return;
	}
	lastp->link = p;
	lastp = p;
}

/*
 * claude: wasm's calling convention *is* "evaluate each argument in
 * order, leave it on the stack" -- there is no REGARG-style first
 * argument special case (there is no register), and no outgoing
 * stack-argument area to build the way regaalloc()'s OINDREG/REGSP
 * offsets do on a real arch. So gargs() collapses to a single loop.
 */
void
gargs(Node *n, Node *tn1, Node *tn2)
{
	USED(tn1);
	USED(tn2);
	if(n == Z)
		return;
	if(n->op == OLIST) {
		gargs(n->left, tn1, tn2);
		gargs(n->right, tn1, tn2);
		return;
	}
	cgen(n, &stackresult);
}

Node*
nodconst(int32 v)
{
	constnode.vconst = v;
	return &constnode;
}

Node*
nod32const(vlong v)
{
	constnode.vconst = v & MASK(32);
	return &constnode;
}

Node*
nodfconst(double d)
{
	fconstnode.fconst = d;
	return &fconstnode;
}

Node*
nodgconst(vlong v, Type *t)
{
	if(!typev[t->etype])
		return nodconst((int32)v);
	vconstnode.vconst = v;
	return &vconstnode;
}

/*
 * claude: nodreg()/regalloc()/regret()/regfree() all just stamp or
 * recognize the single `stackresult` sentinel -- see gc.h's comment.
 * The `reg` parameter every other arch uses to pick a specific
 * hardware register is accepted (to match the shared driver's calls)
 * but ignored: there is only ever one "location" here.
 */
void
nodreg(Node *n, Node *nn, int reg)
{
	USED(reg);
	*n = stackresult;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

void
regret(Node *n, Node *nn)
{
	nodreg(n, nn, 0);
}

void
regalloc(Node *n, Node *tn, Node *o)
{
	USED(o);
	nodreg(n, tn, 0);
}

void
regfree(Node *n)
{
	if(n->op != OREGISTER)
		diag(n, "regfree: not a stack-result node");
}

/*
 * claude: local/parameter index assignment. Every C local/param gets
 * one wasm local slot (even a struct or vlong -- there is no
 * multi-slot layout yet, see docs/notes_wasm.txt), so align() for the
 * variable-placement contexts (Aarg1/Aarg2/Aaut3) just hands out the
 * next integer, ignoring the type entirely. Ael1/Ael2/Asu2 (array/
 * struct byte layout) still use real byte widths, since sizeof() and
 * future struct/array support need them to be correct even though
 * nothing exercises them yet.
 *
 * Aaut3 is called as a single `autoffset = align(autoffset, t,
 * Aaut3)` whose result becomes -autoffset (dcl.c's own convention,
 * shared, not ec's choice) -- so it must both name *and* advance in
 * one call, unlike Aarg1/Aarg2's two-call split. That costs one
 * wasted index between the last param and the first auto; harmless,
 * wasm doesn't mind an unused declared local.
 */
int32
align(int32 stkoff, Type *t, int op)
{
	int32 w;

	switch(op) {
	default:
		diag(Z, "unknown align op %d", op);
		return stkoff;
	case Aarg0:
		nparams = 0;
		return 0;
	case Aarg1:
		nparams++;
		return stkoff;
	case Aarg2:
	case Aaut3:
		return stkoff + 1;
	case Ael1:
		return stkoff;
	case Ael2:
	case Asu2:
		w = t->width;
		if(w == 0)
			w = 1;
		return stkoff + w;
	}
}

int32
maxround(int32 max, int32 v)
{
	if(v > max)
		return v;
	return max;
}

/*
 * claude: recover the wasm local index from a Node's xoffset -- CPARAM
 * locals carry it as-is (0-based, assigned by Aarg1 during argmark()).
 * CAUTO locals carry -autoffset, where autoffset is dcl.c's own
 * counter *reset to 0 after argmark()* (dcl.c line ~653) and then
 * recounted from 1 by Aaut3 for each auto -- a real arch can do that
 * because its params and autos live in disjoint address ranges (FP+
 * vs SP-), but ec's design gives every local (param or auto) one flat
 * wasm slot in the *same* index space. So a bare -o here would
 * silently alias auto #1 onto param #1's slot (and so on) whenever a
 * function has both. Shifting by nparams places every auto right
 * after the last param instead.
 */
static int32
localindex(Node *n)
{
	int32 o;

	o = n->xoffset;
	return o < 0 ? nparams + (-o) - 1 : o;
}

/*
 * claude: no D_OREG/register cross table here -- naddr() only ever
 * has to describe three shapes: a wasm local (a true C auto/param,
 * never address-taken yet), a linear-memory symbol (a C global), or
 * an immediate. See e.out.h's D_* comment for why there's no fourth
 * "address a stack-relative local" case yet.
 */
void
naddr(Node *n, Adr *a)
{
	a->type = D_NONE;
	a->name = D_NONE;
	a->sym = S;
	if(n == Z)
		return;
	switch(n->op) {
	default:
		diag(n, "bad in naddr: %O", n->op);
		break;

	case ONAME:
		switch(n->class) {
		case CAUTO:
		case CPARAM:
			a->type = D_LOCAL;
			a->offset = localindex(n);
			break;
		case CEXTERN:
		case CGLOBL:
			a->type = D_OREG;
			a->name = D_EXTERN;
			a->sym = n->sym;
			a->offset = n->xoffset;
			break;
		case CSTATIC:
			a->type = D_OREG;
			a->name = D_STATIC;
			a->sym = n->sym;
			a->offset = n->xoffset;
			break;
		default:
			diag(n, "bad class in naddr: %d", n->class);
		}
		break;

	case OCONST:
		a->sym = S;
		if(typefd[n->type->etype]) {
			a->type = D_FCONST;
			a->dval = n->fconst;
		} else if(typev[n->type->etype]) {
			a->vval = n->vconst;
			a->type = (int32)a->vval == n->vconst ? D_CONST : D_VCONST;
			a->offset = (int32)a->vval;
		} else {
			a->type = D_CONST;
			a->offset = n->vconst;
		}
		break;
	}
}

/*
 * claude: is this ONAME a true wasm local (fast path), or a C global/
 * static that lives in linear memory? See e.out.h's D_AUTO/D_PARAM
 * comment -- this is always true for CAUTO/CPARAM today because
 * nothing takes a variable's address yet (docs/notes_wasm.txt);
 * ec will need a real address-taken check here once it does.
 */
int
islocal(Node *n)
{
	return n->op == ONAME && (n->class == CAUTO || n->class == CPARAM);
}

/*
 * claude: pushes a C global/static's *address* as a plain i32
 * constant -- reuses naddr()'s existing ONAME->D_OREG mapping (so the
 * D_EXTERN/D_STATIC + sym + offset logic isn't duplicated) and just
 * retags the result as D_CONST, the same trick assemblers/ea/a.y's
 * pushaddr() uses for `$msg(SB)`. Needed because ALOADx/ASTOREx only
 * take a memarg *offset* (see e.out.h) -- the base address must
 * already be on the stack, pushed by a separate ACONSTW first.
 */
void
gaddr(Node *n)
{
	nextpc();
	p->as = ACONSTW;
	p->from.type = D_NONE;
	naddr(n, &p->to);
	p->to.type = D_CONST;
}

/*
 * claude: push n's current value onto the operand stack -- local.get
 * for a true wasm local, or push-address-then-load (offset always 0:
 * the whole address was already folded into gaddr()'s constant) for a
 * C global/static.
 */
void
lload(Node *n)
{
	if(islocal(n)) {
		gins(ALOCALGET, Z, n);
		return;
	}
	gaddr(n);
	gins(loadop(n->type), Z, nodconst(0));
}

/*
 * claude: pop the operand stack's top value into n -- local.set for a
 * true wasm local (no address needed, order doesn't matter), or store
 * for a C global/static. The global case is why this can't be called
 * *after* the value is already on the stack the way a uniform
 * "evaluate then store" flow would: the address must be pushed
 * *before* the value (ASTOREx pops [address, value] in that order),
 * so callers storing an expression's result into a possibly-global
 * lvalue must call gaddr() themselves before evaluating the value --
 * see cgen.c's OAS case, the only caller that needs this to be right.
 */
void
lstore(Node *n)
{
	if(islocal(n)) {
		gins(ALOCALSET, Z, n);
		return;
	}
	gins(storeop(n->type), Z, nodconst(0));
}

/*
 * claude: kept for structural parity with every other arch's gc.h
 * (the shared pgen.c's prototype list expects it), but its one real
 * call site there is codgen()'s REGARG-spill of the first argument,
 * guarded by `if(REGARG >= 0)` -- always false here (see e.out.h's
 * REGARG comment), so this never actually runs. cgen.c does its own
 * gaddr()+lload()/lstore() sequencing directly instead, precisely
 * because gmove()'s simple two-Node shape can't express "push the
 * address first" when the destination turns out to be a global.
 */
void
gmove(Node *f, Node *t)
{
	if(f->op == ONAME) {
		lload(f);
		return;
	}
	if(t->op == ONAME) {
		lstore(t);
		return;
	}
	diag(Z, "gmove: unsupported %O -> %O", f->op, t->op);
}

int
loadop(Type *t)
{
	switch(t->etype) {
	case TCHAR:	return ALOADB;
	case TUCHAR:	return ALOADBU;
	case TSHORT:	return ALOADH;
	case TUSHORT:	return ALOADHU;
	case TVLONG:
	case TUVLONG:	return ALOADQ;
	case TFLOAT:	return ALOADF;
	case TDOUBLE:	return ALOADD;
	default:	return ALOADW;	/* TINT/TUINT/TLONG/TULONG/TIND */
	}
}

int
storeop(Type *t)
{
	switch(t->etype) {
	case TCHAR:
	case TUCHAR:	return ASTOREB;
	case TSHORT:
	case TUSHORT:	return ASTOREH;
	case TVLONG:
	case TUVLONG:	return ASTOREQ;
	case TFLOAT:	return ASTOREF;
	case TDOUBLE:	return ASTORED;
	default:	return ASTOREW;
	}
}

/*
 * claude: arithmetic/compare opcodes need no operand at all (see
 * e.out.h) -- gopcode() just resolves the O* tree op to the matching
 * A* opcode and emits it, nothing more. Only int is wired up for this
 * bootstrap.
 */
void
gopcode(int o, Node *t)
{
	int a;
	int w;

	w = t != Z && t->type != T && typev[t->type->etype];

	switch(o) {
	case OADD:	a = w? AADDQ: AADDW; break;
	case OSUB:	a = w? ASUBQ: ASUBW; break;
	case OMUL:
	case OLMUL:	a = w? AMULQ: AMULW; break;
	case ODIV:	a = w? ADIVQ: ADIVW; break;
	case OLDIV:	a = w? ADIVQU: ADIVWU; break;
	case OMOD:	a = w? AREMQ: AREMW; break;
	case OLMOD:	a = w? AREMQU: AREMWU; break;
	case OAND:	a = w? AANDQ: AANDW; break;
	case OOR:	a = w? AORQ: AORW; break;
	case OXOR:	a = w? AXORQ: AXORW; break;
	case OASHL:	a = w? ASHLQ: ASHLW; break;
	case OASHR:	a = w? ASHRQ: ASHRW; break;
	case OLSHR:	a = w? ASHRQU: ASHRWU; break;
	case OEQ:	a = w? ACMPEQQ: ACMPEQW; break;
	case ONE:	a = w? ACMPNEQ: ACMPNEW; break;
	case OLT:	a = w? ACMPLTQ: ACMPLTW; break;
	case OLE:	a = w? ACMPLEQ: ACMPLEW; break;
	case OGT:	a = w? ACMPGTQ: ACMPGTW; break;
	case OGE:	a = w? ACMPGEQ: ACMPGEW; break;
	case OLO:	a = w? ACMPLTQU: ACMPLTWU; break;
	case OLS:	a = w? ACMPLEQU: ACMPLEWU; break;
	case OHI:	a = w? ACMPGTQU: ACMPGTWU; break;
	case OHS:	a = w? ACMPGEQU: ACMPGEWU; break;
	default:
		diag(Z, "bad in gopcode %O", o);
		return;
	}
	gins(a, Z, Z);
}

void
gins(int a, Node *f, Node *t)
{
	nextpc();
	p->as = a;
	if(f != Z)
		naddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	lastterm = 0;	/* see gbranch()/patch()'s comment on this flag */
}

/*
 * claude: if/else, the first (and so far only) piece of control flow
 * ec supports. See docs/notes_wasm.txt for the general problem this
 * is a restricted case of: pgen.c's gen()/OIF (compilers/cck/pgen.c)
 * is written entirely in terms of flat, PC-based branches --
 * gbranch(OGOTO) emits an unresolved jump and hands back its Prog* in
 * the global `p`; patch(that Prog*, pc) later fills in its target --
 * a model with no notion of "this branch closes a structured
 * construct" at all. Recovering that structure for wasm's block/loop/
 * if nesting in general is a real algorithm (a "relooper"); for
 * if/else specifically (no back-edges) it collapses to something far
 * simpler, worked out here:
 *
 * OIF's own code (traced in full in the design notes) does exactly
 * this, in this order, regardless of whether there's an else:
 *   1. boolgen(cond,1,Z) -- already emits AIF directly (see cgen.c)
 *      and calls pushif() to open a context; the Prog* pgen.c saves
 *      as `sp` is never touched again by ec, it's an opaque token.
 *   2. gen(then-body)
 *   3. IF there's an else: gbranch(OGOTO) [no-op here, see below],
 *      then patch(sp, pc) -- ELSE: just patch(sp, pc) directly.
 *   4. IF there's an else: gen(else-body), then a second
 *      patch(sp, pc).
 *
 * The key realization: wasm's own `if`/`else` construct already skips
 * the untaken branch for free -- reaching the natural end of the
 * then-branch jumps straight past the else-branch to after `end`,
 * with no explicit "goto" needed. So gbranch(OGOTO) here (called only
 * when there's an else, to fake the same "skip the else" a flat-
 * branch arch needs) does not need to emit anything at all; it only
 * needs to *record* that an else is coming, so that patch()'s first
 * call for this if knows to emit AELSE (and expect one more patch()
 * for AENDCTL) instead of closing with AENDCTL immediately.
 *
 * This intentionally assumes gbranch(OGOTO) can currently only mean
 * "the if/else skip" -- true today, since nothing else in ec calls it
 * yet (loops/switch/goto all still diag()). Adding those will need
 * gbranch()/patch() to tell contexts apart properly instead.
 *
 * One more wrinkle, found empirically: codgen() (compilers/cck/
 * pgen.c) unconditionally emits a trailing gbranch(ORETURN) at the
 * very end of every function, whether or not every path already
 * returned explicitly (fact()'s if/else, where both branches return,
 * is exactly this). On a real arch that trailing RET is simply dead
 * code, harmless. wasm's validator statically type-checks every
 * instruction *as ec emits it*, and its unreachable-tracking does not
 * carry across an if/else's own `end` back out to the enclosing
 * block -- so a RET sitting right after a fully-terminal if/else,
 * needing a value nothing left on the stack, is a real validation
 * error, not just dead code (confirmed: V8 rejected it outright).
 * The fix isn't to suppress that RET (canreach, the obvious signal,
 * turns out to already be 0 by the time *any* gbranch(ORETURN) runs --
 * gen()'s own ORETURN case zeroes it before emitting its own RET, so
 * it can't tell "an explicit return, which must emit" from "the
 * trailing one, which mustn't" apart). Instead: track, independently,
 * whether the *last* thing emitted was itself a terminator
 * (`lastterm`), and when closing an if/else whose *both* branches
 * turned out to be terminal, emit a genuine `unreachable` right after
 * `end` -- wasm's unreachable makes everything following it
 * type-polymorphic, so that harmless trailing RET (or anything else)
 * no longer needs the stack to be in any particular state at all.
 */
#define	MAXIFDEPTH	64
enum { IFNOELSE, IFELSEPENDING, IFELSEEMITTED };
static int ifstack[MAXIFDEPTH];
static int ifthenterm[MAXIFDEPTH];	/* did the then-branch end in a terminator? */
static int nifstack;

void
pushif(void)
{
	if(nifstack >= MAXIFDEPTH) {
		diag(Z, "if/else nested too deeply");
		return;
	}
	ifstack[nifstack++] = IFNOELSE;
}

void
gbranch(int o)
{
	switch(o) {
	case ORETURN:
		nextpc();
		p->as = ARET;
		lastterm = 1;
		return;
	case OGOTO:
		if(nifstack > 0 && ifstack[nifstack-1] == IFNOELSE) {
			ifstack[nifstack-1] = IFELSEPENDING;
			ifthenterm[nifstack-1] = lastterm;
			lastterm = 0;	/* the else-branch starts fresh/reachable */
			return;
		}
		/* claude: no control flow beyond if/else yet,
		 * see docs/notes_wasm.txt's "Open questions for ec". */
		diag(Z, "goto/loop/switch control flow not implemented yet in ec");
		nextpc();
		p->as = AUNREACHABLE;
		lastterm = 1;
		return;
	default:
		diag(Z, "bad in gbranch %O", o);
		nextpc();
		p->as = AUNREACHABLE;
		lastterm = 1;
	}
}

void
patch(Prog *op, int32 pc)
{
	USED(op);
	USED(pc);
	if(nifstack <= 0) {
		diag(Z, "patch: unexpected control-flow resolution (no open if)");
		return;
	}
	if(ifstack[nifstack-1] == IFELSEPENDING) {
		nextpc();
		p->as = AELSE;
		ifstack[nifstack-1] = IFELSEEMITTED;
		return;
	}
	nextpc();
	p->as = AENDCTL;
	if(ifstack[nifstack-1] == IFELSEEMITTED && ifthenterm[nifstack-1] && lastterm) {
		/* both branches terminated: see the file comment above */
		nextpc();
		p->as = AUNREACHABLE;
		lastterm = 1;
	} else {
		/* claude: no-else case (an if without an else always falls
		 * through when the condition is false, matching pgen.c's own
		 * `canreach = canreach || oldreach` -- always 1 here) or a
		 * with-else case where at least one branch fell through. */
		lastterm = 0;
	}
	nifstack--;
}

void
gpseudo(int a, Sym *s, Node *n)
{
	nextpc();
	p->as = a;
	p->from.type = D_OREG;
	p->from.sym = s;
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;

	/*
	 * claude: codgen() (compilers/cck/pgen.c) calls gpseudo(ATEXT,...)
	 * directly, not through a per-arch gtext() hook the way
	 * src/cmd/cc's own copy of codgen() does for ic/7c/vc -- an
	 * earlier draft of this file had a dead gtext() that never ran
	 * because of that lineage difference. So ASIGNATURE (wasm-only,
	 * see e.out.h's comment) is emitted right here instead, the one
	 * place that's guaranteed to run immediately after every ATEXT.
	 * nparams is already final by now: dcl.c's argmark()/walkparam()
	 * (via align()'s Aarg0/Aarg1 cases) walk all of a function's
	 * parameters before codgen() ever runs. Int-only for this
	 * bootstrap (every param/result becomes 'W' = i32).
	 */
	if(a == ATEXT) {
		char sig[NSNAME];
		int i;

		memset(sig, 0, sizeof(sig));
		for(i = 0; i < nparams && i < NSNAME-2; i++)
			sig[i] = 'W';
		sig[i] = thisfn->link->etype == TVOID ? 'V' : 'W';

		nextpc();
		p->as = ASIGNATURE;
		p->from.type = D_OREG;
		p->from.sym = s;
		p->from.name = D_EXTERN;
		p->to.type = D_SCONST;
		memmove(p->to.sval, sig, NSNAME);
		pc--;	/* metadata about the TEXT above, not its own instruction */
	}
}

/*
 * claude: every backend owns this table -- the frontend's own type
 * width table, indexed by T* etype. Ordering/values match ic/txt.c's
 * (rv32 variant): wasm32 has 4-byte pointers, same width as int, same
 * as riscv32.
 */
schar	ewidth[NTYPE] =
{
	-1,		/* [TXXX] */
	SZ_CHAR,	/* [TCHAR] */
	SZ_CHAR,	/* [TUCHAR] */
	SZ_SHORT,	/* [TSHORT] */
	SZ_SHORT,	/* [TUSHORT] */
	SZ_INT,		/* [TINT] */
	SZ_INT,		/* [TUINT] */
	SZ_LONG,	/* [TLONG] */
	SZ_LONG,	/* [TULONG] */
	SZ_VLONG,	/* [TVLONG] */
	SZ_VLONG,	/* [TUVLONG] */
	SZ_FLOAT,	/* [TFLOAT] */
	SZ_DOUBLE,	/* [TDOUBLE] */
	SZ_IND,		/* [TIND] */
	0,		/* [TFUNC] */
	-1,		/* [TARRAY] */
	0,		/* [TVOID] */
	-1,		/* [TSTRUCT] */
	-1,		/* [TUNION] */
	SZ_INT,		/* [TENUM] */
};

/*
 * claude: which types may be silently cast to which -- a C-language
 * property, not really arch-specific, except that BIND (pointer-width
 * compatible) only belongs on TINT/TUINT/TLONG/TULONG when pointers
 * and int are the same width, same condition ic/txt.c uses to choose
 * between its 32- and 64-bit ncast[] tables. wasm32 pointers are
 * 4 bytes, same as int, so this matches ic's rv32 (not rv64) variant.
 */
long	ncast[NTYPE] =
{
	0,				/* [TXXX] */
	BCHAR|BUCHAR,			/* [TCHAR] */
	BCHAR|BUCHAR,			/* [TUCHAR] */
	BSHORT|BUSHORT,			/* [TSHORT] */
	BSHORT|BUSHORT,			/* [TUSHORT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TINT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TUINT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TLONG] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TULONG] */
	BVLONG|BUVLONG,			/* [TVLONG] */
	BVLONG|BUVLONG,			/* [TUVLONG] */
	BFLOAT,				/* [TFLOAT] */
	BDOUBLE,			/* [TDOUBLE] */
	BLONG|BULONG|BIND,		/* [TIND] */
	0,				/* [TFUNC] */
	0,				/* [TARRAY] */
	0,				/* [TVOID] */
	BSTRUCT,			/* [TSTRUCT] */
	BUNION,				/* [TUNION] */
	0,				/* [TENUM] */
};

/*
 * claude: "external register" allocation -- a C `register`-class
 * variable that a real arch tries to keep in a hardware register for
 * the variable's whole lifetime, spanning multiple statements/calls
 * (unlike regalloc()'s short-lived temps). Meaningless for wasm (no
 * register file), so always declines; the frontend falls back to
 * ordinary CAUTO storage whenever this returns 0.
 */
long
exreg(Type *t)
{
	USED(t);
	return 0;
}
