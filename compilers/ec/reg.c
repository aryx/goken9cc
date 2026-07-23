/*
 * ec/reg.c -- the wasm "structuring" pass.
 *
 * Every other arch's regopt() turns a flat Prog list using pseudo-
 * registers into one using real hardware registers -- pure
 * optimization on already-correct code. wasm has no register file, so
 * there is nothing to optimize, but codgen() (compilers/cck/pgen.c)
 * still calls regopt(sp) unconditionally once per function, right
 * after that function's whole flat body (gbranch()/patch()'s ABR/
 * ABRIF placeholders, with to.offset holding a raw target pc exactly
 * like a real arch's D_BRANCH -- see txt.c) has been generated. ec
 * reuses that same hook for the analogous wasm-specific
 * transformation: turning the flat, PC-addressed branch list into
 * properly nested wasm block/loop/br/br_if.
 *
 * This works, without needing a general "relooper" for arbitrary
 * goto, because gen() only ever produces *reducible* control flow for
 * if/while/for/break/continue (the constructs ec's frontend recognizes
 * -- switch and arbitrary goto still diag() elsewhere). It does NOT
 * mean every branch-target span nests cleanly by construction, though
 * -- two real cases need fixing up before block/loop insertion:
 *   - A forward branch's *own* operand computation (an ABRIF's
 *     condition) can start well before the branch itself, and must
 *     stay inside whatever block wraps that branch's target (see
 *     safeopen()'s comment) -- otherwise the wrapping block resets
 *     the operand stack out from under it.
 *   - Two branch targets can genuinely partially overlap in the flat
 *     list -- e.g. an if/else's own "skip the else" jump sits inside
 *     the *test's* wrapping block (which closes right at the start of
 *     the else) but needs to reach past the whole if/else (see
 *     extendscopes()'s comment).
 * Both are fixed by widening a block's *open* point outward (always
 * safe -- see the comments where each happens); what this can't fix
 * is a branch whose *target* falls strictly inside another scope's
 * body while the branch itself sits outside it (as opposed to before
 * it) -- exactly what a C `for`/`do while`'s own entry jump needs
 * (skipping the first iteration's increment/test, landing partway
 * into what the back-edge treats as the loop). That's a real
 * unsupported case today (see emit()'s validatescopes() check) --
 * fixing it needs an actual code-shape change (loop rotation:
 * duplicating the test so entry never has to jump into a loop's
 * middle), not just more interval-insertion logic.
 *
 * Four passes over the function's flat Prog list:
 *
 *   1. Jump-threading (resolve()/threadjumps()): any branch whose
 *      target is itself a bare unconditional ABR gets its target
 *      replaced by that ABR's own target, repeated to a fixed point.
 *      gen() emits exactly this kind of "trampoline" for breakpc/
 *      continpc: a pc value is captured *before* the placeholder that
 *      will later be patched to the real destination (see pgen.c's
 *      OWHILE/OFOR), so two placeholders often chain together before
 *      reaching the real target.
 *
 *   2. Dead code marking + compaction (markdead()/compact()): after
 *      threading, any Prog that is neither a fallthrough target of
 *      the previous live instruction (i.e. that instruction wasn't
 *      itself a terminator) nor a real branch target is dead. This
 *      eliminates the now-unreachable trampolines from (1), *and*
 *      elides the redundant trailing ARET codgen() always appends
 *      whether or not the function's body already returned on every
 *      path -- the general form of a problem an earlier version of
 *      this file solved one construct (if/else) at a time by
 *      inserting AUNREACHABLE; simply never emitting the dead
 *      instruction at all is simpler still.
 *
 *   3. Scope construction (buildscopes()): every surviving branch
 *      target becomes a wasm structural boundary. A forward target
 *      (branch's own position precedes the target) needs an
 *      enclosing block whose end lands there; a backward target
 *      (target at or before the branch) needs an enclosing loop
 *      starting there.
 *
 *   4. Emission (emit()): sweep live positions in order, opening and
 *      closing scopes (as real ABLOCK/ALOOP/AENDCTL Progs) as their
 *      spans dictate, and rewriting each branch's to.offset from a
 *      raw pc into the block/loop nesting depth wasm's br/br_if
 *      actually need.
 */
#include "gc.h"

enum
{
	SBLOCK,
	SLOOP,
	MAXDEPTH	= 128,
};

typedef struct Scope Scope;
struct Scope
{
	int	kind;
	int32	open;	/* live index where this scope is entered */
	int32	close;	/* live index right after this scope is exited */
};

static Prog**
collect(Prog *sp, int32 *pn)
{
	Prog *pp;
	Prog **list;
	int32 n, i;

	n = 0;
	for(pp = sp; pp != P; pp = pp->link)
		n++;
	list = alloc(n * sizeof(Prog*));
	i = 0;
	for(pp = sp; pp != P; pp = pp->link)
		list[i++] = pp;
	*pn = n;
	return list;
}

static int32
resolve(Prog **list, int32 n, int32 base, int32 target)
{
	int32 idx, seen;

	seen = 0;
	for(;;) {
		idx = target - base;
		if(idx < 0 || idx > n) {
			diag(Z, "ec: branch target out of range");
			return target;
		}
		if(idx == n || list[idx]->as != ABR)
			return target;
		target = list[idx]->to.offset;
		if(++seen > n) {
			diag(Z, "ec: cycle in branch chain");
			return target;
		}
	}
}

static void
threadjumps(Prog **list, int32 n, int32 base)
{
	int32 i;

	for(i = 0; i < n; i++)
		if(list[i]->as == ABR || list[i]->as == ABRIF)
			list[i]->to.offset = resolve(list, n, base, list[i]->to.offset);
}

/*
 * claude: dead[i] set means list[i] can never execute -- neither by
 * falling through (its predecessor, among earlier non-dead entries,
 * was itself a terminator) nor by being branched to (after
 * threading). Reaching a real branch target always resets
 * reachability, matching wasm's own rule that a block/loop's `end`
 * resets reachability regardless of how the code leading to it ended.
 */
static char*
markdead(Prog **list, int32 n, int32 base)
{
	char *dead, *istarget;
	int32 i, idx;
	int reachable;

	dead = alloc(n);
	istarget = alloc(n);
	memset(dead, 0, n);
	memset(istarget, 0, n);

	for(i = 0; i < n; i++) {
		if(list[i]->as != ABR && list[i]->as != ABRIF)
			continue;
		idx = list[i]->to.offset - base;
		if(idx >= 0 && idx < n)
			istarget[idx] = 1;
	}

	reachable = 1;
	for(i = 0; i < n; i++) {
		if(istarget[i])
			reachable = 1;
		dead[i] = !reachable;
		if(dead[i])
			continue;
		switch(list[i]->as) {
		case ARET:
		case ABR:
		case AUNREACHABLE:
			reachable = 0;
			break;
		}
	}
	return dead;
}

/*
 * claude: compact away dead entries into a parallel array indexed by
 * live position, plus liveidx[] mapping an original index to its live
 * one (-1 if dead) -- everything past this point reasons purely in
 * live-position terms. Linked-list relinking happens later, in
 * emit(), which rebuilds ->link from scratch anyway while splicing in
 * the new ABLOCK/ALOOP/AENDCTL Progs.
 */
static Prog**
compact(Prog **list, int32 n, char *dead, int32 *liveidx, int32 *pm)
{
	Prog **live;
	int32 i, m;

	m = 0;
	for(i = 0; i < n; i++)
		liveidx[i] = dead[i] ? -1 : m++;
	live = alloc(m * sizeof(Prog*));
	for(i = 0; i < n; i++)
		if(!dead[i])
			live[liveidx[i]] = list[i];
	*pm = m;
	return live;
}

/*
 * claude: live[k]->to.offset still holds an original (pre-compaction)
 * pcid at this point -- rewrite every branch's target to the matching
 * live index before buildscopes()/emit() (below) need to compare
 * positions directly.
 */
static void
retarget(Prog **live, int32 m, int32 base, int32 *liveidx)
{
	int32 k, orig;

	for(k = 0; k < m; k++) {
		if(live[k]->as != ABR && live[k]->as != ABRIF)
			continue;
		orig = live[k]->to.offset - base;
		live[k]->to.offset = liveidx[orig];
	}
}

/*
 * claude: a wasm block/loop resets what's visible on the operand
 * stack to its own declared params (none, for the void blocks/loops
 * this file only ever emits) -- a value computed *before* the block
 * opens is not available to instructions strictly inside it, even
 * though the underlying value stack still physically holds it
 * (confirmed empirically: a hand-assembled module with `<value>;
 * block; br_if 0; end` fails to validate, "not enough arguments on
 * the stack for br_if"). ABRIF is the only branch with an operand
 * (its condition), and boolgen() (cgen.c) always emits that
 * condition's own computation as an unbroken run immediately
 * preceding it -- so the block wrapping an ABRIF's target must open
 * at or before wherever *that run* starts, not at the ABRIF itself.
 *
 * live[j]->height (txt.c's stackheight, recorded per instruction --
 * see gc.h's comment) gives the exact operand-stack height right
 * before live[j] runs, so the nearest certainly-safe position at or
 * before a branch is simply the closest earlier position with height
 * 0. An earlier version of this used a cheaper heuristic (scan
 * backward for the previous ATEXT/ASIGNATURE/ABR/ABRIF/ARET) that
 * seemed sound but wasn't: it doesn't recognize that an ordinary
 * ALOCALSET *also* leaves height 0, so it walked further back than
 * necessary and occasionally produced a block whose span crossed
 * another one instead of nesting inside it (found empirically on
 * pick()'s if/else, where neither branch returns). A plain ABR (no
 * operand, e.g. a `break`) already has height 0 at its own position,
 * so this returns pos itself for that case -- no widening needed.
 */
static int32
safeopen(Prog **live, int32 pos)
{
	int32 j;

	for(j = pos; j >= 0; j--)
		if(live[j]->height == 0)
			return j;
	return 0;
}

/*
 * claude: one Scope per distinct branch target -- a block for every
 * target reached by a forward branch, a loop for every target reached
 * by a backward one. blockid[k]/loopid[k] record which scope (if any)
 * of each kind opens at live position k, so emit() can look up "which
 * scope does this specific branch need to reach" directly.
 *
 * Spans computed here can *partially* overlap another scope (e.g.
 * if/else's own "skip the else" jump sits inside the test's own
 * wrapping block but needs to reach past it, out to after the whole
 * if/else) -- extendscopes() (below) fixes those up into proper
 * nesting before emit() runs.
 */
static Scope*
buildscopes(Prog **live, int32 m, int32 *nscope, int32 *blockid, int32 *loopid)
{
	int32 *blockopen, *loopclose;
	int32 k, srck, dstk, cand;
	Scope *scope;
	int32 ns;

	blockopen = alloc(m * sizeof(int32));
	loopclose = alloc(m * sizeof(int32));
	for(k = 0; k < m; k++) {
		blockopen[k] = -1;
		loopclose[k] = -1;
		blockid[k] = -1;
		loopid[k] = -1;
	}

	for(srck = 0; srck < m; srck++) {
		if(live[srck]->as != ABR && live[srck]->as != ABRIF)
			continue;
		dstk = live[srck]->to.offset;
		if(dstk > srck) {
			cand = safeopen(live, srck);
			if(blockopen[dstk] == -1 || cand < blockopen[dstk])
				blockopen[dstk] = cand;
		} else {
			if(loopclose[dstk] == -1 || srck > loopclose[dstk])
				loopclose[dstk] = srck;
		}
	}

	scope = alloc(2 * m * sizeof(Scope));
	ns = 0;
	for(k = 0; k < m; k++) {
		if(blockopen[k] != -1) {
			scope[ns].kind = SBLOCK;
			scope[ns].open = blockopen[k];
			scope[ns].close = k;
			blockid[k] = ns;
			ns++;
		}
		if(loopclose[k] != -1) {
			scope[ns].kind = SLOOP;
			scope[ns].open = k;
			scope[ns].close = loopclose[k] + 1;
			loopid[k] = ns;
			ns++;
		}
	}
	*nscope = ns;
	return scope;
}

/*
 * claude: fix up any pair of scopes that partially overlap (one's
 * open falls strictly inside the other's span but its close falls
 * strictly outside it) into proper nesting, by pulling the offender's
 * open point out to enclose the whole scope it was cutting through.
 * Safe repeatedly: widening a scope's open can only ever ADD it to
 * more enclosing scopes, matching the same "wrap a bit more than
 * strictly necessary is harmless" reasoning safeopen() already relies
 * on. Needed because a branch's own safe-open position (safeopen())
 * only accounts for *its own* operand visibility, not for whatever
 * *other* scope happens to sit between it and its target -- e.g.
 * if/else's "skip the else" jump sits inside the test's own wrapping
 * block (which closes at the start of the else) but must itself
 * reach all the way past the whole if/else.
 *
 * Only ever widens a *block*'s open (j's kind, below): a loop's open
 * is its own back-edge target, a real position loopid[] is indexed
 * by, and buildscopes() never records a second entry for a moved-to
 * position -- moving one open would silently break that lookup.
 * Nothing seen so far needs a loop's open widened for this same
 * reason (its whole body, including anything nested in it, already
 * closes at or before the loop's own close by construction).
 */
static void
extendscopes(Scope *scope, int32 nscope)
{
	int32 i, j;
	int changed;

	changed = 1;
	while(changed) {
		changed = 0;
		for(i = 0; i < nscope; i++) {
			for(j = 0; j < nscope; j++) {
				if(i == j || scope[j].kind != SBLOCK)
					continue;
				if(scope[i].open <= scope[j].open && scope[j].open < scope[i].close
				&& scope[j].close > scope[i].close
				&& scope[j].open > scope[i].open) {
					scope[j].open = scope[i].open;
					changed = 1;
				}
			}
		}
	}
}

/*
 * claude: extendscopes() (above) can only fix a scope by widening its
 * *open* point outward -- there's no equivalent move for a scope
 * whose *close* (a real branch target, not adjustable) falls strictly
 * inside a loop while the branch reaching it sits entirely outside
 * that loop. That's exactly a C `for`/`do while`'s own entry jump:
 * it lands partway into what the back-edge treats as the loop body
 * (skipping the first iteration's increment or test), which wasm's
 * block/loop nesting simply can't express as a plain forward branch.
 * Caught here with a clear diagnostic instead of silently emitting a
 * module that fails wasm validation -- see the file comment's note on
 * loop rotation being the real fix, not yet implemented.
 */
static void
validatescopes(Scope *scope, int32 nscope)
{
	int32 i, j;

	for(i = 0; i < nscope; i++) {
		if(scope[i].kind != SLOOP)
			continue;
		for(j = 0; j < nscope; j++) {
			if(scope[j].kind != SBLOCK)
				continue;
			if(scope[j].open < scope[i].open && scope[i].open < scope[j].close
			&& scope[j].close < scope[i].close) {
				diag(Z, "ec: unsupported control flow -- a branch lands inside "
					"a loop body from outside it (C for/do-while entry is not "
					"implemented yet, see docs/notes_wasm.txt)");
				return;
			}
		}
	}
}

static int
scopecmp(Scope *a, Scope *b)
{
	if(a->open != b->open)
		return a->open - b->open;
	if(a->close != b->close)
		return b->close - a->close;	/* wider span opens first (outermost) */
	/* claude: identical span (the common while-loop shape, where the
	 * safeopen() widening above can make the loop-exit block's own
	 * span exactly coincide with the loop's) -- block must be the
	 * outer of the two: br (loop-outer) means "jump to loop start",
	 * br (block-outer) means "jump past block end", and a br_if
	 * escaping the loop needs the latter. */
	if(a->kind != b->kind)
		return a->kind == SBLOCK ? -1 : 1;
	return 0;
}

static void
sortscopes(int32 *order, Scope *scope, int32 nscope)
{
	int32 i, j, t;

	for(i = 0; i < nscope; i++)
		order[i] = i;
	for(i = 1; i < nscope; i++) {
		t = order[i];
		j = i;
		while(j > 0 && scopecmp(&scope[order[j-1]], &scope[t]) > 0) {
			order[j] = order[j-1];
			j--;
		}
		order[j] = t;
	}
}

static Prog*
newprog(int as)
{
	Prog *np;

	np = alloc(sizeof(Prog));
	*np = zprog;
	np->as = as;
	return np;
}

/*
 * claude: the actual sweep -- walk live positions in order, closing
 * any scopes whose span ends here (innermost first: whatever's on top
 * of the stack, by construction of properly-nested spans, is always
 * exactly what closes here if anything does), opening any that start
 * here (outermost first, per sortscopes()'s order), computing each
 * branch's final depth against the *current* stack, then splicing the
 * live instruction itself (and any new ABLOCK/ALOOP/AENDCTL Progs)
 * into a fresh linked list. Returns the new first Prog -- always
 * live[0] itself (never dead, never a branch target, and per
 * safeopen()'s comment, never anything a scope opens in front of
 * either); *plast comes back as the new last Prog, which the caller
 * uses to fix up the global lastp.
 *
 * claude: live[0] is *not* actually this function's ATEXT -- codgen()
 * (compilers/cck/pgen.c) does `gpseudo(ATEXT,...); sp = p;`, and
 * gpseudo() (txt.c) internally emits ASIGNATURE right after ATEXT for
 * the same symbol, leaving the global `p` (and so `sp`) pointing at
 * ASIGNATURE once gpseudo() returns. So regopt(sp) -- and everything
 * in this file -- only ever sees the range starting at ASIGNATURE;
 * ATEXT itself sits one Prog further back, outside collect()'s walk
 * entirely, still linking straight to the original ASIGNATURE object.
 * That's why live[0] must never move or have anything spliced before
 * it: nothing here has a handle back to ATEXT to redirect if it did.
 */
static Prog*
emit(Prog **live, int32 m, Scope *scope, int32 nscope, int32 *blockid, int32 *loopid, Prog **plast)
{
	int32 *order;
	int32 stack[MAXDEPTH];
	int32 stacktop;
	int32 pos, si, k, sid, depth, j;
	Prog *first, *last, *np;

	order = alloc((nscope ? nscope : 1) * sizeof(int32));
	sortscopes(order, scope, nscope);

	first = P;
	last = P;
	stacktop = 0;
	si = 0;

	for(pos = 0; pos < m; pos++) {
		while(stacktop > 0 && scope[stack[stacktop-1]].close == pos) {
			np = newprog(AENDCTL);
			if(first == P) first = np; else last->link = np;
			last = np;
			stacktop--;
		}
		while(si < nscope && scope[order[si]].open == pos) {
			sid = order[si];
			np = newprog(scope[sid].kind == SLOOP ? ALOOP : ABLOCK);
			if(first == P) first = np; else last->link = np;
			last = np;
			if(stacktop >= MAXDEPTH)
				diag(Z, "ec: control-flow nesting too deep");
			else
				stack[stacktop++] = sid;
			si++;
		}

		if(live[pos]->as == ABR || live[pos]->as == ABRIF) {
			k = live[pos]->to.offset;
			sid = (k > pos) ? blockid[k] : loopid[k];
			depth = -1;
			if(sid < 0) {
				diag(Z, "ec: branch has no matching scope");
			} else {
				for(j = stacktop-1; j >= 0; j--)
					if(stack[j] == sid) {
						depth = (stacktop-1) - j;
						break;
					}
				if(depth < 0)
					diag(Z, "ec: branch target scope not on stack");
			}
			live[pos]->to.offset = depth;
			live[pos]->to.type = D_BRANCH;
		}

		if(first == P) first = live[pos]; else last->link = live[pos];
		last = live[pos];
	}

	while(stacktop > 0) {
		np = newprog(AENDCTL);
		last->link = np;
		last = np;
		stacktop--;
	}

	last->link = P;
	*plast = last;
	return first;
}

void
regopt(Prog *sp)
{
	Prog **list, **live;
	Prog *newlast;
	int32 n, m, nscope, base;
	char *dead;
	int32 *liveidx, *blockid, *loopid;
	Scope *scope;

	list = collect(sp, &n);
	base = list[0]->pcid;

	threadjumps(list, n, base);
	dead = markdead(list, n, base);

	liveidx = alloc(n * sizeof(int32));
	live = compact(list, n, dead, liveidx, &m);
	retarget(live, m, base, liveidx);

	blockid = alloc(m * sizeof(int32));
	loopid = alloc(m * sizeof(int32));
	scope = buildscopes(live, m, &nscope, blockid, loopid);
	extendscopes(scope, nscope);
	validatescopes(scope, nscope);

	emit(live, m, scope, nscope, blockid, loopid, &newlast);

	/* sp itself (live[0]) never moves or dies, so whatever pointed to
	 * it in the whole-program list still does; only the global
	 * "most recently appended Prog" pointer, which the *next*
	 * function's codgen() will extend via ->link, needs updating. */
	lastp = newlast;
}
