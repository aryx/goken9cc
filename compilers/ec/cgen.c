/*
 * ec/cgen.c -- expression codegen.
 *
 * claude: nothing here resembles ic/cgen.c's register-juggling
 * structure (regalloc a temp, evaluate a subexpression into it,
 * gopcode, regfree) because there is no register pressure to manage:
 * a stack machine's own operand stack already holds however many
 * pending values are in flight, in exactly the order a left-to-right
 * recursive-descent evaluation produces them -- even across nested
 * function calls, since each call's result is consumed by whatever
 * operator is waiting for it before anything else gets pushed. See
 * docs/notes_wasm.txt.
 *
 * Structure: cgen(n, nn) is the public entry point (n's result goes
 * into nn, or is dropped if nn==Z, or -- if nn is the `stackresult`
 * sentinel -- is simply left on the stack). It's built on rval(n),
 * an internal helper with a simpler contract: leave exactly one wasm
 * value on the stack, nothing else. Assignment is the one case that
 * can't be built on rval() as a black box: storing into a C global
 * needs its address pushed *before* the value (see txt.c's lstore()
 * comment), so OAS gets its own sequencing here instead.
 */
#include "gc.h"

static void rval(Node*);

static int
isvoidt(Type *t)
{
	return t == T || t->etype == TVOID;
}

void
cgen(Node *n, Node *nn)
{
	if(n == Z)
		return;

	if(n->op == OAS) {
		if(n->left->op != ONAME) {
			diag(n, "cgen: assignment target not implemented yet: %O", n->left->op);
			return;
		}
		if(islocal(n->left)) {
			rval(n->right);
			gins(ALOCALSET, Z, n->left);
		} else {
			gaddr(n->left);		/* address first: see lstore()'s comment */
			rval(n->right);
			gins(storeop(n->left->type), Z, nodconst(0));
		}
		if(nn != Z) {
			if(nn->op != OREGISTER) {
				diag(n, "cgen: assignment-as-value into a non-stack target not implemented yet");
				return;
			}
			/* claude: no `dup` in wasm MVP -- re-fetch rather
			 * than duplicate the value already consumed above. */
			lload(n->left);
		}
		return;
	}

	rval(n);

	if(nn == Z) {
		if(!isvoidt(n->type))
			gins(ADROP, Z, Z);
		return;
	}
	if(nn->op == OREGISTER)
		return;		/* already exactly where cgen()'s caller wants it */
	lstore(nn);
}

static void
rval(Node *n)
{
	Node *l, *r;

	if(n == Z)
		return;
	l = n->left;
	r = n->right;

	switch(n->op) {
	case OCONST:
		if(typefd[n->type->etype])
			gins(n->type->etype == TFLOAT ? ACONSTF : ACONSTD, Z, n);
		else if(typev[n->type->etype])
			gins(ACONSTQ, Z, n);
		else
			gins(ACONSTW, Z, n);
		return;

	case ONAME:
		lload(n);
		return;

	case OFUNC:
		/* claude: only a direct call to a named function is
		 * implemented (l is an ONAME) -- an indirect call through
		 * a function pointer would need ACALLIND and a type-
		 * section index, neither wired up yet. */
		if(l->op != ONAME) {
			diag(n, "cgen: indirect call not implemented yet");
			return;
		}
		{
			/* claude: variable arity (depends on the callee's own
			 * signature), so tracked here rather than through
			 * txt.c's stackdelta() table -- gargs() already leaves
			 * stackheight correct for however many args it just
			 * pushed (each arg's own cgen() call tracks itself), so
			 * this only needs to account for the call itself: pop
			 * back to before the args, push one result unless void. */
			int32 before = stackheight;
			gargs(r, Z, Z);
			gins(ACALL, Z, l);
			stackheight = before + (l->type->link->etype == TVOID ? 0 : 1);
		}
		return;

	case OADD:
	case OSUB:
	case OMUL:
	case OLMUL:
	case ODIV:
	case OLDIV:
	case OMOD:
	case OLMOD:
	case OAND:
	case OOR:
	case OXOR:
	case OASHL:
	case OASHR:
	case OLSHR:
	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGT:
	case OGE:
	case OLO:
	case OLS:
	case OHI:
	case OHS:
		rval(l);
		rval(r);
		gopcode(n->op, l);
		return;

	case OCAST:
		/* claude: int-only for this bootstrap -- a same-width or
		 * widening cast between integer types needs no instruction
		 * at all yet (wasm's i32 already covers char/short/int/long
		 * uniformly); real narrowing/float conversions aren't wired
		 * up (see e.out.h's AWRAPQ/AEXTW/ACONVWF/etc family). */
		rval(l);
		return;

	default:
		diag(n, "cgen: not implemented yet: %O", n->op);
	}
}

/*
 * claude: boolgen() generates either a branch (nn==Z, used by an
 * if/while/for's own condition) or a materialized 0/1 value (nn!=Z,
 * e.g. `int ok = a < b;`).
 *
 * The branch form: bcomplex() (compilers/cck/pgen.c) is the only
 * caller, always as `boolgen(cond, 1, Z)` -- confirmed against
 * ic/cgen.c's own boolgen, whose otrue=1 case means "branch away when
 * the condition is FALSE, fall through when TRUE" (used for if's
 * then-branch, while/for's exit test, alike). A wasm br_if (ABRIF)
 * has the *opposite* polarity from that: it branches when the popped
 * value is nonzero/true. So this negates exactly when otrue (i.e. in
 * the one case pgen.c actually uses), to make the ABRIF fire on
 * false -- backwards from the value-form's own `if(!otrue) negate`
 * just below, which keeps its ordinary "produce true" meaning.
 *
 * ABRIF is emitted as a flat, unresolved placeholder exactly like
 * gbranch(OGOTO)'s ABR (see txt.c) -- pgen.c patches its target later
 * the same way, and reg.c's structuring pass resolves it into a real
 * wasm depth once the function's whole flat body is known.
 */
void
boolgen(Node *n, int otrue, Node *nn)
{
	rval(n);

	if(nn == Z) {
		if(otrue)
			gins(ATESTW, Z, Z);	/* i32.eqz */
		nextpc();
		p->as = ABRIF;
		p->height = stackheight;
		stackheight--;	/* pops its condition -- see txt.c's stackdelta() comment */
		return;
	}

	if(!otrue)
		gins(ATESTW, Z, Z);
	if(nn->op == OREGISTER)
		return;
	lstore(nn);
}
