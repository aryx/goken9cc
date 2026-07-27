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
static void asop(Node*);
static void incdec(Node*);
static void callvariadic(Node*, Node*);

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
		/* claude: *p = val -- same "address before value" ordering
		 * as the C-global case just below (ASTOREx pops [address,
		 * value]), but the address comes from evaluating the
		 * pointer sub-expression (rval(n->left->left)) rather than
		 * gaddr(), since there's no symbol/offset to fold into a
		 * link-time constant here. */
		if(n->left->op == OIND) {
			rval(n->left->left);
			rval(n->right);
			gins(storeop(n->left->type), Z, nodconst(0));
		} else if(n->left->op != ONAME) {
			diag(n, "cgen: assignment target not implemented yet: %O", n->left->op);
			return;
		} else if(isvarargparam(n->left)) {
			/* claude: print_nofloat_no64.c's printf() never assigns
			 * to its own `s` (only reads it, and takes its address)
			 * -- no test exercises this, and guessing the semantics
			 * would be exactly the kind of unrequested scope this
			 * project's conventions warn against. See
			 * docs/notes_wasm.txt's variadic-call-ABI section. */
			diag(n, "cgen: assignment to a variadic function's own named parameter not implemented yet");
			return;
		} else if(islocal(n->left)) {
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
			 * than duplicate the value already consumed above.
			 * rval(OIND) re-evaluates the pointer and reloads,
			 * exactly the same re-fetch idea as lload() below. */
			if(n->left->op == OIND)
				rval(n->left);
			else
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
	case OAS:
		/* claude: a chained/nested assignment used as a value, e.g.
		 * print_nofloat_no64.c's vprintf(): `lp = p = s;` parses as
		 * OAS(lp, OAS(p, s)) -- the inner OAS is *rval()'s* problem
		 * (an ordinary binary op's rval(r) call, or asop()'s own
		 * rval(n->right), might land on one), but all the actual
		 * assignment sequencing lives in cgen()'s own OAS handling
		 * above, not here. Delegate to it with the stackresult
		 * sentinel: cgen()'s own "no dup, re-fetch" tail (nn!=Z &&
		 * nn->op==OREGISTER) is exactly "leave the assigned value on
		 * the stack", rval()'s own contract. */
		cgen(n, &stackresult);
		return;

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
		if(isvariadic(l->type)) {
			callvariadic(r, l);
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

	case OADDR:
		/* claude: a C global/static's address is already a link-time
		 * constant (see gaddr()'s own comment) -- string literals hit
		 * this path too, since cck/com.c's OSTRING case turns a string
		 * into an ONAME (class CSTATIC, sym=symstring) that decays to
		 * `char*` via an implicit OADDR, same as any other array-to-
		 * pointer decay. A *local*'s address is a different story: it
		 * would need a real stack slot (the shadow-stack redesign
		 * docs/notes_wasm.txt's "Open questions" describes), not just
		 * retagging a naddr() result, so that case still diag()s. */
		if(l->op != ONAME) {
			diag(n, "cgen: address-of not implemented yet: %O", l->op);
			return;
		}
		if(islocal(l)) {
			diag(n, "cgen: address-of a local not implemented yet (needs the shadow-stack redesign, see docs/notes_wasm.txt)");
			return;
		}
		if(isvarargparam(l)) {
			/* claude: &s inside a variadic function -- exactly
			 * vprintf()'s own `&s+1`. s already lives at a known,
			 * fixed offset (4*l->xoffset) from the incoming
			 * argument-block pointer (see isvarargparam()'s comment,
			 * txt.c), so its address is just that pointer plus the
			 * offset -- 0 for printf's own `s`, since it's the only
			 * named param, but computed in general in case a future
			 * variadic function has more than one. */
			gins(ALOCALGET, Z, nodargptr());
			if(l->xoffset != 0) {
				gins(ACONSTW, Z, nodconst(4*l->xoffset));
				gins(AADDW, Z, Z);
			}
			return;
		}
		gaddr(l);
		return;

	case OIND:
		/* claude: *p -- evaluate the pointer, then load through it
		 * at offset 0 (the memarg-offset mechanism already proven
		 * correct by last session's STOREx-operand-order fix; see
		 * cgen()'s OAS case for the write side). */
		rval(l);
		gins(loadop(n->type), Z, nodconst(0));
		return;

	case OASADD:
	case OASSUB:
	case OASMUL:
	case OASLMUL:
	case OASDIV:
	case OASLDIV:
	case OASMOD:
	case OASLMOD:
	case OASAND:
	case OASOR:
	case OASXOR:
	case OASASHL:
	case OASASHR:
	case OASLSHR:
		asop(n);
		return;

	case OPOSTINC:
	case OPOSTDEC:
	case OPREINC:
	case OPREDEC:
		incdec(n);
		return;

	default:
		diag(n, "cgen: not implemented yet: %O", n->op);
	}
}

/*
 * claude: base (non-assigning) tree-op gopcode() already knows how to
 * emit for each OAS* compound-assignment variant -- cck/com.c keeps
 * `n->op` as the OAS* form all the way through (rewriting only the
 * signed/unsigned choice, e.g. OASDIV->OASLDIV for an unsigned
 * operand), so ec's backend is the one that has to peel the "compound"
 * part off before calling gopcode(), same as every other case here
 * that reuses gopcode() for its own binary-op dispatch.
 */
static int
asopbase(int o)
{
	switch(o) {
	case OASADD:	return OADD;
	case OASSUB:	return OSUB;
	case OASMUL:	return OMUL;
	case OASLMUL:	return OLMUL;
	case OASDIV:	return ODIV;
	case OASLDIV:	return OLDIV;
	case OASMOD:	return OMOD;
	case OASLMOD:	return OLMOD;
	case OASAND:	return OAND;
	case OASOR:	return OOR;
	case OASXOR:	return OXOR;
	case OASASHL:	return OASHL;
	case OASASHR:	return OASHR;
	case OASLSHR:	return OLSHR;
	}
	diag(Z, "asopbase: bad op %O", o);
	return OADD;
}

/*
 * claude: l OP= r -- result is the *new* value (C semantics: `x =
 * (v /= 16)` uses v's post-division value). Only an ONAME lvalue is
 * implemented (matches print_nofloat_no64.c's own usage, e.g.
 * printhex's `v /= 16` on a local uint32 -- nothing in the shared
 * sources ever compound-assigns through a pointer/array lvalue); a
 * more general lvalue diag()s rather than guessing, same spirit as
 * cgen()'s own OAS/OADDR cases above.
 *
 * "No dup in wasm MVP" idiom throughout: the local case uses
 * ALOCALTEE (store-and-leave-a-copy, a real wasm instruction, so no
 * re-fetch needed there); the global case re-fetches via a second
 * lload() instead, exactly like OAS's own result path.
 */
static void
asop(Node *n)
{
	Node *l;
	int op;

	l = n->left;
	op = asopbase(n->op);

	if(l->op != ONAME) {
		diag(n, "cgen: compound-assignment target not implemented yet: %O", l->op);
		return;
	}
	if(isvarargparam(l)) {
		/* claude: same reasoning as OAS's own isvarargparam() branch
		 * in cgen() above -- gaddr() below only knows CSTATIC/
		 * CEXTERN/CGLOBL, and would silently mis-address a variadic
		 * function's own named param if this weren't caught first. */
		diag(n, "cgen: compound assignment to a variadic function's own named parameter not implemented yet");
		return;
	}
	if(islocal(l)) {
		lload(l);
		rval(n->right);
		gopcode(op, l);
		gins(ALOCALTEE, Z, l);
		return;
	}
	gaddr(l);		/* address first: see lstore()'s comment */
	lload(l);
	rval(n->right);
	gopcode(op, l);
	gins(storeop(l->type), Z, nodconst(0));
	lload(l);		/* re-fetch: the expression's result is the new value */
}

/*
 * claude: ++l/l++/--l/l--. Only an ONAME lvalue is implemented, same
 * reasoning as asop() above (print_nofloat_no64.c/helloprintf.c only
 * ever inc/dec a plain local int or pointer *variable*, e.g. vprintf's
 * `p++`, printhex's `--i`, fact_iter's `i--` -- never `*p` or `a[i]`).
 * Pointer operands scale by the pointee's width (matches every other
 * arch's own cgen.c, e.g. ic/cgen.c's OPOSTINC case).
 *
 * POST leaves the *old* value (computed and left first, then the
 * store sequence runs "underneath" it on the stack -- no dup needed,
 * the old value just sits below whatever the store pushes/pops).
 * PRE leaves the *new* value, same ALOCALTEE/re-fetch split as asop().
 */
static void
incdec(Node *n)
{
	Node *l;
	int32 w;

	l = n->left;
	if(l->op != ONAME) {
		diag(n, "cgen: inc/dec target not implemented yet: %O", l->op);
		return;
	}
	if(isvarargparam(l)) {
		/* claude: same reasoning as asop()'s own isvarargparam()
		 * guard just above. */
		diag(n, "cgen: inc/dec of a variadic function's own named parameter not implemented yet");
		return;
	}

	w = 1;
	if(l->type->etype == TIND)
		w = l->type->link->width;
	if(n->op == OPOSTDEC || n->op == OPREDEC)
		w = -w;

	switch(n->op) {
	case OPOSTINC:
	case OPOSTDEC:
		if(islocal(l)) {
			lload(l);	/* old value: the result, left for later */
			lload(l);	/* re-fetch to compute the new value from */
			gins(ACONSTW, Z, nodconst(w));
			gopcode(OADD, l);
			gins(ALOCALSET, Z, l);
		} else {
			lload(l);	/* old value: the result, left for later */
			gaddr(l);	/* address first: see lstore()'s comment */
			lload(l);
			gins(ACONSTW, Z, nodconst(w));
			gopcode(OADD, l);
			gins(storeop(l->type), Z, nodconst(0));
		}
		return;

	case OPREINC:
	case OPREDEC:
		if(islocal(l)) {
			lload(l);
			gins(ACONSTW, Z, nodconst(w));
			gopcode(OADD, l);
			gins(ALOCALTEE, Z, l);	/* store, leave the new value */
		} else {
			gaddr(l);	/* address first: see lstore()'s comment */
			lload(l);
			gins(ACONSTW, Z, nodconst(w));
			gopcode(OADD, l);
			gins(storeop(l->type), Z, nodconst(0));
			lload(l);	/* re-fetch: the result is the new value */
		}
		return;
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

/*
 * claude: how many actual arguments a call site passes -- OLIST is a
 * binary right-leaning list, same shape gargs() (txt.c) already
 * recurses over; this just counts leaves instead of emitting them.
 */
static int
countvariadicargs(Node *n)
{
	if(n == Z)
		return 0;
	if(n->op == OLIST)
		return countvariadicargs(n->left) + countvariadicargs(n->right);
	return 1;
}

/*
 * claude: store each actual argument into the caller-reserved
 * shadow-stack buffer, in order, one 4-byte slot per argument
 * (int-only bootstrap, same "one slot per value" convention as
 * everywhere else in this file) -- idx is threaded through the
 * recursion rather than a static counter, so this stays reentrant
 * the same way gargs()'s own OLIST recursion already is. Each
 * store is the exact `GLOBALGET(SP); <value>; STOREx $off` shape
 * already proven correct by last session's STOREx-operand-order fix
 * (tests/s/mini/storeorder_wasm.s): the address (SP, freshly
 * re-read -- it never changes mid-sequence, but re-reading it is
 * simpler than caching a copy across cgen()'s own recursive calls)
 * is pushed *before* the value, matching ASTOREx's [address, value]
 * pop order.
 */
static int
storevariadicargs(Node *n, int idx)
{
	if(n == Z)
		return idx;
	if(n->op == OLIST) {
		idx = storevariadicargs(n->left, idx);
		idx = storevariadicargs(n->right, idx);
		return idx;
	}
	gspglobal(AGLOBALGET);
	rval(n);
	gins(storeop(n->type), Z, nodconst(4*idx));
	return idx+1;
}

/*
 * claude: the variadic-call ABI's caller side (see
 * docs/notes_wasm.txt's variadic-call-ABI section for the full
 * picture, and isvarargparam()/nodargptr() in txt.c for the callee
 * side). wasm's own `call` is arity/type-checked against the
 * callee's declared signature -- there is no way to just push extra
 * values the way a real ISA's stack-based ABI would let a variadic
 * call site do, so instead every actual argument (fixed *and*
 * variadic) is written into a small buffer reserved on the shadow
 * stack, and the call itself passes exactly one wasm argument: the
 * buffer's base address. This is not a wasm-specific invention --
 * it's the same lowering LLVM's wasm32 target / Emscripten use for
 * real C varargs.
 *
 * SPGLOBAL is only decremented for the duration of this one call
 * (restored immediately after ACALL returns, not before -- the
 * callee reads this memory *during* the call) -- exactly the
 * prologue/epilogue discipline docs/notes_wasm.txt's shadow-stack
 * section describes, just performed at the call site rather than in
 * a callee's own entry/exit (nothing here needs a *general*
 * per-function shadow-stack frame yet, since no ordinary local is
 * address-taken -- see cgen.c's OADDR case and txt.c's isvarargparam()
 * comment for why the general case stays out of scope).
 */
static void
callvariadic(Node *r, Node *l)
{
	int nargs;
	int32 size;
	int32 before;

	nargs = countvariadicargs(r);
	size = 4*nargs;

	gspglobal(AGLOBALGET);
	gins(ACONSTW, Z, nodconst(size));
	gins(ASUBW, Z, Z);
	gspglobal(AGLOBALSET);

	storevariadicargs(r, 0);

	/* claude: same "before/after" accounting cgen()'s ordinary OFUNC
	 * case uses -- before is the height with none of *this* call's
	 * own wasm-level arguments pushed yet, and this call always has
	 * exactly one (the base address just below). */
	before = stackheight;
	gspglobal(AGLOBALGET);
	gins(ACALL, Z, l);
	stackheight = before + (l->type->link->etype == TVOID ? 0 : 1);

	gspglobal(AGLOBALGET);
	gins(ACONSTW, Z, nodconst(size));
	gins(AADDW, Z, Z);
	gspglobal(AGLOBALSET);
}
