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

/* claude: see the comment above naddr()'s isvarargparam()/argptrnode
 * section further down -- declared up here (initialized by ginit(),
 * used by lload()/cgen.c's OADDR case) purely because ginit() runs
 * before any of that code is defined in file order. */
static Node argptrnode;

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
	stackheight = 0;
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

	/* claude: see argptrnode's own comment (above naddr()) -- a
	 * synthetic "local 0" reference, shared by every variadic
	 * function this translation unit compiles. */
	argptrnode = znode;
	argptrnode.op = ONAME;
	argptrnode.class = CPARAM;
	argptrnode.xoffset = 0;
	argptrnode.type = types[TIND];

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
	p->pcid = pc;	/* see reg.c's structuring pass: breakpc/continpc-style
			 * snapshots taken *before* the matching gbranch() call
			 * are, by construction, exactly the pcid this Prog is
			 * about to receive. */
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
 * claude: does this function's Type have a trailing "..."? dcl.c's
 * fnproto1()'s ODOTDOT case terminates the param chain with a TDOT
 * type node, so walking `down` and checking for it is exactly how
 * tmerge() (cck/dcl.c) itself already recognizes a variadic prototype
 * when merging a forward declaration against a definition. Called
 * both on `thisfn` (the function being compiled, see gpseudo()'s
 * ATEXT case and localindex() below) and on a call site's callee type
 * (cgen.c's OFUNC case) -- both see the same TDOT via minilibc.h's
 * shared `extern void printf(char *s, ...);` declaration.
 */
int
isvariadic(Type *t)
{
	for(t = t->down; t != T; t = t->down)
		if(t->etype == TDOT)
			return 1;
	return 0;
}

/*
 * claude: the ASIGNATURE char for one C Type -- 'W'=i32 (int/short/
 * char/pointer, the common case), 'Q'=i64 (vlong/uvlong), 'F'=f32,
 * 'D'=f64. Shared by gpseudo()'s ATEXT case below for both a
 * function's own param types (thisfn->down's chain) and its result
 * type (thisfn->link) -- reuses cck/sub.c's typefd[]/typev[] tables
 * (already indexed by Type->etype, and already how every other piece
 * of the shared frontend classifies a type into float-vs-vlong-vs-int)
 * rather than duplicating that classification here.
 */
static int
sigchar(Type *t)
{
	if(typefd[t->etype])
		return t->etype == TFLOAT ? 'F' : 'D';
	if(typev[t->etype])
		return 'Q';
	return 'W';
}

/*
 * claude: how many *real* wasm param locals a function has -- nparams
 * (dcl.c's own count of *named* params) for an ordinary function, but
 * always exactly 1 for a variadic one (the incoming argument-block
 * pointer; see isvarargparam()'s comment and docs/notes_wasm.txt's
 * variadic-call-ABI section). Used both for ASIGNATURE's arity
 * (gpseudo()'s ATEXT case) and for shifting autos past the real
 * params (localindex() below) -- nparams itself must NOT be used for
 * either once a function is variadic, since it counts named params,
 * which is a different (and, in general, smaller) number.
 */
static int
wasmnparams(void)
{
	return isvariadic(thisfn) ? 1 : nparams;
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
 * function has both. Shifting by wasmnparams() places every auto
 * right after the last *real* wasm param instead -- for an ordinary
 * function that's nparams, same as always; for a variadic function
 * it's 1 (see wasmnparams()'s comment), since its own named params
 * (o >= 0 below) are never actually reached for a variadic function's
 * CPARAM in the first place -- see isvarargparam().
 */
static int32
localindex(Node *n)
{
	int32 o;

	o = n->xoffset;
	return o < 0 ? wasmnparams() + (-o) - 1 : o;
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
 * comment -- this is true for every CAUTO (nothing takes an auto's
 * address yet, docs/notes_wasm.txt) and for a CPARAM *unless* it's a
 * named parameter of a variadic function, in which case it's
 * isvarargparam() below instead: it doesn't live in a wasm local at
 * all, real or otherwise, since the whole point of the variadic-call
 * ABI is that such a function's *only* true wasm local is the
 * incoming argument-block pointer (local 0).
 */
int
islocal(Node *n)
{
	return n->op == ONAME &&
		(n->class == CAUTO || (n->class == CPARAM && !isvariadic(thisfn)));
}

/*
 * claude: a named parameter of a variadic function (e.g. printf's own
 * `s`) -- islocal()'s complement for CPARAM. Never true for a CAUTO:
 * a variadic function's own locals (e.g. printf's `byte *arg;`) are
 * ordinary wasm locals exactly like any other function's, just
 * indexed starting after the single real param (see wasmnparams()).
 * See docs/notes_wasm.txt's variadic-call-ABI section for the full
 * picture: the caller writes `s` and all "..." actuals into a
 * contiguous shadow-stack buffer and passes just its base address;
 * this predicate is what makes the callee read `s` back out of that
 * buffer (via the incoming pointer, argptrnode below) instead of
 * expecting its own wasm local.
 */
int
isvarargparam(Node *n)
{
	return n->op == ONAME && n->class == CPARAM && isvariadic(thisfn);
}

/*
 * claude: a synthetic reference to wasm local 0 -- every variadic
 * function's *sole* true wasm param, the incoming argument-block
 * pointer (see isvarargparam()'s comment above). argptrnode itself is
 * declared near the top of this file (ginit() initializes it before
 * any of this is defined in file order); built as a plain CPARAM
 * ONAME with xoffset 0 so naddr()'s existing ONAME/CPARAM case
 * (D_LOCAL, offset via localindex()) already does the right thing
 * with zero new naddr()/e.out.h plumbing -- localindex() returns o
 * unchanged for any o >= 0, so this really does resolve to local
 * index 0, exactly like an ordinary function's own first named
 * parameter would. Exposed to cgen.c via nodargptr() rather than the
 * Node itself, the same accessor-function style nodconst()/
 * nodfconst() already use for their own static sentinels.
 */
Node*
nodargptr(void)
{
	return &argptrnode;
}

static int stackdelta(int);

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
	/* claude: a real bug lived here -- this never set p->height or
	 * updated stackheight the way every other instruction-emitting
	 * helper (gins(), gspglobal()) does, even though it genuinely
	 * pushes one wasm value (a symbol's address). Masked until now
	 * because every prior caller (lload()'s global-read path, OAS's
	 * non-local write path) immediately follows gaddr() with another
	 * gins() call whose own height/stackheight bookkeeping happened
	 * to leave the *net* effect looking consistent by the time
	 * anything downstream (reg.c's structuring pass) inspected it.
	 * Using OADDR as a bare call argument (cgen.c's OFUNC/gargs()
	 * path, nothing else calls gaddr() without a following gins())
	 * has no such follow-up instruction to absorb the omission,
	 * leaving stackheight permanently one low from here on -- reg.c's
	 * structuring pass then misreads the (now-wrong) height sequence,
	 * corrupting the block/loop nesting it emits. Confirmed via a
	 * minimal repro (a 2-arg call whose first argument is `&global`)
	 * that failed wasm validation with "not enough arguments for
	 * call" before this fix. See tests/c/mini2/regress_wasm.c's
	 * deref() regression test (its call site passes `&gscratch`
	 * as a bare argument, exactly the repro shape). */
	nextpc();
	p->as = ACONSTW;
	p->from.type = D_NONE;
	p->height = stackheight;
	naddr(n, &p->to);
	p->to.type = D_CONST;
	stackheight += stackdelta(ACONSTW);
}

/*
 * claude: emit a bare AGLOBALGET/AGLOBALSET of SPGLOBAL -- the
 * variadic-call ABI's own building block (cgen.c's OFUNC case), used
 * to reserve/restore a caller-allocated argument-block buffer on the
 * shadow stack (see docs/notes_wasm.txt's "What a shadow stack
 * actually is" and variadic-call-ABI sections). Hand-built exactly
 * like gaddr() above rather than going through gins()+a Node/naddr(),
 * because there's no existing Node shape that means "the SP global"
 * to reuse (unlike nodargptr(), which could piggyback on the ordinary
 * ONAME/CPARAM path) -- SPGLOBAL is a D_GLOBAL, not a D_LOCAL, and
 * naddr() has no case that produces one from any Node op today.
 */
void
gspglobal(int as)
{
	nextpc();
	p->as = as;
	p->from.type = D_NONE;
	p->height = stackheight;
	p->to.type = D_GLOBAL;
	p->to.offset = SPGLOBAL;
	stackheight += stackdelta(as);
}

/*
 * claude: push n's current value onto the operand stack -- local.get
 * for a true wasm local, push-address-then-load through the incoming
 * argument-block pointer for a variadic function's own named param
 * (see isvarargparam()'s comment; offset is 4*n->xoffset, one 4-byte
 * slot per named param, matching the caller-side layout cgen.c's
 * OFUNC case builds), or push-address-then-load (offset always 0: the
 * whole address was already folded into gaddr()'s constant) for a C
 * global/static.
 */
void
lload(Node *n)
{
	if(islocal(n)) {
		gins(ALOCALGET, Z, n);
		return;
	}
	if(isvarargparam(n)) {
		gins(ALOCALGET, Z, nodargptr());
		gins(loadop(n->type), Z, nodconst(4*n->xoffset));
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
 * bootstrap: a float64 value already works as a constant, a call
 * argument/result, and a function parameter/result (see
 * docs/notes_wasm.txt's "A function's real ASIGNATURE..." section),
 * but no test source performs actual float *arithmetic* yet, so this
 * diag()s instead of silently picking an integer opcode for a float
 * operand -- the AADDF/AADDD/etc family (e.out.h, txt.c's stackdelta())
 * is implemented and ready for whoever wires this in first.
 */
void
gopcode(int o, Node *t)
{
	int a;
	int w;

	if(t != Z && t->type != T && typefd[t->type->etype]) {
		diag(Z, "gopcode: float arithmetic not implemented yet: %O", o);
		return;
	}
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

/*
 * claude: net wasm operand-stack effect of one instruction -- see
 * Prog.height's comment (gc.h) for why reg.c needs this exactly,
 * rather than a cheaper "was the previous thing a branch" heuristic
 * (which an earlier version of this file tried and got wrong: it
 * treated an ordinary ALOCALSET as *not* resetting height, so a
 * value-computing branch's own condition computation looked like it
 * needed to reach further back than it actually did, occasionally
 * producing a block whose span crossed another one instead of
 * nesting inside it). ACALL and ARET are variable-arity (a call's
 * argument/result count depends on the callee; a return's on the
 * current function's own type) and are tracked at their own call
 * sites instead (cgen.c's rval()/OFUNC case, gbranch()'s ORETURN
 * case) -- both unreachable here, listed only so an accidental
 * gins(ACALL,...)/gins(ARET,...) call is caught rather than silently
 * mistracked.
 */
static int
stackdelta(int a)
{
	switch(a) {
	case ACONSTW: case ACONSTQ: case ACONSTF: case ACONSTD:
	case ALOCALGET: case AGLOBALGET:
	case AMEMSIZE:
		return 1;
	case ALOCALSET: case AGLOBALSET:
	case ADROP:
		return -1;
	case ALOCALTEE:
	case AMEMGROW:
		return 0;
	case ALOADB: case ALOADBU: case ALOADH: case ALOADHU:
	case ALOADW: case ALOADQ: case ALOADBQ: case ALOADBUQ:
	case ALOADHQ: case ALOADHUQ: case ALOADWQ: case ALOADWUQ:
	case ALOADF: case ALOADD:
		return 0;	/* pop address, push value */
	case ASTOREB: case ASTOREH: case ASTOREW: case ASTOREWQ:
	case ASTOREQ: case ASTOREF: case ASTORED:
		return -2;	/* pop address and value */
	case AADDW: case ASUBW: case AMULW: case ADIVW: case ADIVWU:
	case AREMW: case AREMWU: case AANDW: case AORW: case AXORW:
	case ASHLW: case ASHRW: case ASHRWU: case AROLW: case ARORW:
	case ACMPEQW: case ACMPNEW: case ACMPLTW: case ACMPLTWU:
	case ACMPGTW: case ACMPGTWU: case ACMPLEW: case ACMPLEWU:
	case ACMPGEW: case ACMPGEWU:
	case AADDQ: case ASUBQ: case AMULQ: case ADIVQ: case ADIVQU:
	case AREMQ: case AREMQU: case AANDQ: case AORQ: case AXORQ:
	case ASHLQ: case ASHRQ: case ASHRQU: case AROLQ: case ARORQ:
	case ACMPEQQ: case ACMPNEQ: case ACMPLTQ: case ACMPLTQU:
	case ACMPGTQ: case ACMPGTQU: case ACMPLEQ: case ACMPLEQU:
	case ACMPGEQ: case ACMPGEQU:
	case AADDF: case ASUBF: case AMULF: case ADIVF: case AMINF: case AMAXF: case ACOPYSGNF:
	case ACMPEQF: case ACMPNEF: case ACMPLTF: case ACMPGTF: case ACMPLEF: case ACMPGEF:
	case AADDD: case ASUBD: case AMULD: case ADIVD: case AMIND: case AMAXD: case ACOPYSGND:
	case ACMPEQD: case ACMPNED: case ACMPLTD: case ACMPGTD: case ACMPLED: case ACMPGED:
		return -1;	/* binary: pop 2, push 1 */
	case ACLZW: case ACTZW: case APOPCNTW: case ATESTW:
	case ACLZQ: case ACTZQ: case APOPCNTQ: case ATESTQ:
	case AABSF: case ANEGF: case ASQRTF: case ACEILF: case AFLOORF: case ATRUNCF: case ANEARF:
	case AABSD: case ANEGD: case ASQRTD: case ACEILD: case AFLOORD: case ATRUNCD: case ANEARD:
	case AWRAPQ: case AEXTW: case AEXTWU:
	case ACONVWF: case ACONVWUF: case ACONVWD: case ACONVWUD:
	case ACONVQF: case ACONVQUF: case ACONVQD: case ACONVQUD:
	case ATRUNCFW: case ATRUNCFWU: case ATRUNCFQ: case ATRUNCFQU:
	case ATRUNCDW: case ATRUNCDWU: case ATRUNCDQ: case ATRUNCDQU:
	case APROMOTE: case ADEMOTE:
	case AREINTWF: case AREINTFW: case AREINTQD: case AREINTDQ:
		return 0;	/* unary: pop 1, push 1 */
	case ACALL:
		/* claude: variable arity -- gins(ACALL,...)'s caller (cgen.c's
		 * rval()/OFUNC) overwrites stackheight itself right after this
		 * runs, using the callee's actual signature. This 0 is never
		 * the real answer, just a placeholder so gins() has something
		 * to add without diag()ing on every single call site. */
		return 0;
	}
	diag(Z, "stackdelta: unhandled opcode %A", a);
	return 0;
}

void
gins(int a, Node *f, Node *t)
{
	nextpc();
	p->as = a;
	p->height = stackheight;
	if(f != Z)
		naddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	stackheight += stackdelta(a);
}

/*
 * claude: gbranch()/patch() are, on purpose, almost identical to
 * ic/txt.c's real-arch versions: a flat, PC-addressed branch model.
 * gbranch(OGOTO) emits an unresolved ABR and hands back its Prog* in
 * the global `p`; patch(that Prog*, pc) later fills in `to.offset`
 * with a raw target pc, exactly like a real arch's D_BRANCH. This
 * covers if/else, while/for, and break/continue uniformly (the same
 * gbranch(OGOTO)/patch() calls pgen.c already makes for all of them --
 * see compilers/cck/pgen.c's OIF/OWHILE/OFOR/OBREAK/OCONTINUE), unlike
 * an earlier version of this file that special-cased if/else's own
 * call pattern directly here and had no way to generalize to loops.
 *
 * boolgen()'s branch form (cgen.c) emits the matching conditional
 * placeholder, ABRIF, the same way.
 *
 * The wasm-specific part -- turning this flat, PC-addressed branch
 * list into properly nested block/loop/br/br_if, *and* eliding the
 * trailing gbranch(ORETURN) codgen() always appends whether or not
 * the function already returned on every path -- happens once per
 * function, after gen() has produced the whole flat list: see
 * reg.c's regopt(), which plays that role the same way a real arch's
 * regopt() post-processes its own flat Prog list (into real
 * registers instead of wasm structure). Full algorithm and rationale
 * in reg.c and docs/notes_wasm.txt.
 */
void
gbranch(int o)
{
	switch(o) {
	case ORETURN:
		nextpc();
		p->as = ARET;
		p->height = stackheight;
		stackheight -= (thisfn->link->etype == TVOID) ? 0 : 1;
		return;
	case OGOTO:
		nextpc();
		p->as = ABR;
		p->height = stackheight;	/* no operand: doesn't change height */
		return;
	default:
		diag(Z, "bad in gbranch %O", o);
		nextpc();
		p->as = AUNREACHABLE;
		p->height = stackheight;
	}
}

void
patch(Prog *op, int32 pc)
{
	op->to.offset = pc;
	op->to.type = D_BRANCH;
}

void
gpseudo(int a, Sym *s, Node *n)
{
	Prog *ptext;

	nextpc();
	p->as = a;
	p->from.type = D_OREG;
	p->from.sym = s;
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	/*
	 * claude: ic/txt.c's own gpseudo() (which this is modeled on, see
	 * gc.h's file comment) does `if(a == ADATA || a == AGLOBL) pc--;`
	 * here, so consecutive data chunks/globals don't inflate its pc
	 * counter -- harmless there since a real arch's regopt() never
	 * indexes anything by raw pcid. Deliberately dropped for ec: a
	 * string-literal argument's ADATA (com.c's OSTRING case, via
	 * outstring()) is emitted *while* the enclosing function's own
	 * statements are being walked, not as a separate pre/post pass, so
	 * it becomes one more Prog in *that function's* own flat list --
	 * exactly the list reg.c's collect()/resolve() index by raw pcid
	 * (list[target-base]), which silently assumes every entry's pcid is
	 * unique and position-matching. `pc--` here duplicated the ADATA's
	 * pcid onto whatever Prog came right after it (e.g. the ACONSTW
	 * pushing the string's address for a call argument), desyncing
	 * every later index in that function from its true pcid by one.
	 * Confirmed via a minimal repro: a `for` loop containing two
	 * `switch` statements, preceded by a call passing a string literal
	 * (e.g. `debug("x")`), miscompiled into a genuine cyclic branch
	 * chain (reg.c's resolve() diag "cycle in branch chain") instead of
	 * valid wasm -- exactly print_nofloat_no64.c's vprintf() shape
	 * (tests/c/mini2). Same reasoning as the ASIGNATURE case just below
	 * (also deliberately not `pc--`), just for a pseudo-op that can
	 * appear mid-function instead of only right after ATEXT. Top-level
	 * AGLOBL/ADATA (gclean()'s own end-of-file loop, gextern()'s
	 * globals) never reach regopt() at all -- pcid is reg.c-internal
	 * only (never serialized to the .e file), so leaving those dense
	 * too costs nothing.
	 */

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
	 * parameters before codgen() ever runs. Each param/result gets its
	 * real sigchar() ('W'/'Q'/'F'/'D'), not a hardcoded 'W' -- needed
	 * once a function can return or take a float64 (e.g. tests/c/mini2/
	 * test.c's test_float()); see sigchar()'s own comment.
	 *
	 * wasmnparams(), not nparams, decides the arity: for a variadic
	 * function this must be exactly 1 (the incoming argument-block
	 * pointer), never however many *named* C params it has -- see
	 * wasmnparams()'s own comment and docs/notes_wasm.txt's
	 * variadic-call-ABI section. For an ordinary function the two are
	 * the same, so this is a no-op change on the non-variadic path.
	 */
	if(a == ATEXT) {
		char sig[NSNAME];
		int i, np;
		Type *t;

		/* claude: a real bug lived here -- ptext saves the ATEXT
		 * Prog itself, because the nextpc() call below (for
		 * ASIGNATURE) overwrites the global `p` to point at that new
		 * Prog instead. codgen() (cck/pgen.c) does `gpseudo(ATEXT,
		 * ...); sp = p;` immediately after this function returns,
		 * expecting `p` to still be the ATEXT Prog it just asked for
		 * -- true for every other arch's own gpseudo()/gtext(), which
		 * never emit a second Prog inside the same call. Without
		 * restoring `p` before returning, codgen()'s later `sp->
		 * to.offset += maxargsafe` (its own mechanism for folding
		 * regsalloc()'s scratch-local reservations into the ATEXT
		 * frame size, see swt.c's regsalloc()) was silently mutating
		 * the *ASIGNATURE* Prog's unused to.offset field instead of
		 * the real frame size -- harmless as long as maxargsafe was
		 * always 0, which it was for every function until swit1()
		 * (below) became the first thing in ec to ever call
		 * regsalloc(). Confirmed via a minimal switch-statement repro
		 * that failed wasm validation with "invalid local index"
		 * (the scratch local's index was never actually declared)
		 * before this fix. */
		ptext = p;

		stackheight = 0;
		memset(sig, 0, sizeof(sig));
		np = wasmnparams();
		/* claude: a variadic function's sole real wasm param is the
		 * synthetic argument-block pointer (argptrnode, TIND), never
		 * whatever its own named C params are typed -- always 'W',
		 * same as before. An ordinary function's params come straight
		 * from thisfn->down's chain, in order (see fnproto()/dcl.c's
		 * `t->down = fnproto(n)`, the same chain isvariadic() already
		 * walks looking for its trailing TDOT). */
		if(isvariadic(thisfn)) {
			for(i = 0; i < np && i < NSNAME-2; i++)
				sig[i] = 'W';
		} else {
			t = thisfn->down;
			for(i = 0; i < np && i < NSNAME-2; i++) {
				sig[i] = t != T ? sigchar(t) : 'W';
				if(t != T)
					t = t->down;
			}
		}
		sig[i] = thisfn->link->etype == TVOID ? 'V' : sigchar(thisfn->link);

		nextpc();
		p->as = ASIGNATURE;
		p->from.type = D_OREG;
		p->from.sym = s;
		p->from.name = D_EXTERN;
		p->to.type = D_SCONST;
		memmove(p->to.sval, sig, NSNAME);
		/* claude: deliberately *not* `pc--` here (unlike the ADATA/
		 * AGLOBL case above) -- reg.c's structuring pass needs every
		 * Prog's pcid, including this one, to be a genuinely unique,
		 * position-matching identity; decrementing pc after this made
		 * the *next* Prog created collide with ASIGNATURE's own pcid. */

		p = ptext;
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
