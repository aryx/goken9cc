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
 * safe -- see the comments where each happens); neither fixes a
 * branch whose *target* falls strictly inside another scope's body
 * while the branch itself sits outside it (as opposed to before it)
 * -- exactly what a C `for`/`do while`'s own entry jump needs
 * (skipping the first iteration's increment/test, landing partway
 * into what the back-edge treats as the loop). That case needs an
 * actual code-shape change instead: rotateloops() (below) physically
 * reorders the flat instructions into the standard "loop rotation"
 * shape (see rotateonce()'s own comment) so the entry jump never has
 * to land mid-loop again. It has to run on the already-compacted,
 * dead-code-free list (see findrotation()'s comment for why running
 * it any earlier reproduces the very violation it's trying to fix).
 * validatescopes() stays as a safety net for anything that shape
 * doesn't cover (arbitrary goto is still just diag()'d before it gets
 * this far, so nothing should reach it today).
 *
 * Five passes over the function's flat Prog list:
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
 *   3. Loop rotation (findrotation()/rotateonce()/rotateloops()): find
 *      and fix the C `for`/`do while` shape described above, by
 *      physically reordering the now dead-code-free flat instructions
 *      -- see rotateonce()'s own comment for the exact rearrangement
 *      and why it's behaviorally identical to the original.
 *
 *   4. Scope construction (buildscopes()): every surviving branch
 *      target becomes a wasm structural boundary. A forward target
 *      (branch's own position precedes the target) needs an
 *      enclosing block whose end lands there; a backward target
 *      (target at or before the branch) needs an enclosing loop
 *      starting there.
 *
 *   5. Emission (emit()): sweep live positions in order, opening and
 *      closing scopes (as real ABLOCK/ALOOP/AENDCTL Progs) as their
 *      spans dictate, and rewriting each branch's to.offset from a
 *      raw pc into the block/loop nesting depth wasm's br/br_if
 *      actually need.
 */
#include "gc.h"

static Prog*	newprog(int as);

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
 * claude: find one C `for`/`do while` shape: a forward ABR at position
 * `i` (< Lopen) whose target `T` lands strictly inside some backward-
 * branch-defined loop [Lopen,Dclose) -- i.e. exactly the pattern
 * validatescopes() (further down) diag()s on, but detected here, in
 * time to actually fix it (see rotateonce()'s comment for why).
 * Operates on already-live, already-0-based indices (run *after*
 * markdead()/compact(), not before): an earlier version ran on the
 * raw pre-compaction list and kept "fixing" the dead breakpc/continpc
 * trampolines gen() leaves behind (see txt.c's/pgen.c's OWHILE/OFOR
 * comments) right alongside the real entry jump -- each of those
 * trampolines, once correctly retargeted to wherever its destination
 * code now lives, becomes *itself* a forward-branch-into-a-loop's-
 * interior, since it's structurally identical to the real entry jump
 * (both are unconditional branches from outside the loop into it).
 * That reproduced the violation forever instead of fixing it once.
 * Dead code has no such trampolines left (they're exactly what
 * markdead() elides), so running here is both correct and simpler --
 * one call to run at, no base/pcid arithmetic needed.
 *
 * A `continue` inside the loop is *also* a backward ABR targeting the
 * same Lopen as the loop's own automatic back-edge (both ultimately
 * mean "go do the increment") -- so for a given Lopen, Dclose must be
 * 1 + the *largest* matching backward source (matching buildscopes()'
 * own loopclose[] logic just below), not whichever one happens to be
 * found first; using an earlier `continue` as Dclose instead of the
 * real, later back-edge produced a wrong (too-small) loop bound.
 *
 * The backward branch defining a loop's own Dclose isn't always an
 * unconditional ABR either: for a `for` nested directly in another
 * `for`'s body, the outer loop's own automatic back-edge (gen()'s
 * "body finished normally" ABR) is itself unreachable-via-fallthrough
 * dead code once the inner loop is the entire body (the inner loop's
 * own trailing ABR already is the thing right before it) and gets
 * elided by markdead() -- what's left targeting the outer Lopen is
 * the *inner* loop's own conditional test-exit (ABRIF), which just
 * happens to double as "outer loop, keep going" once it falls out of
 * the inner loop. So both ABR and ABRIF count as backward-branch
 * candidates here, matching buildscopes()' own check just below.
 *
 * For nested for/do-while (one directly in the other's body), more
 * than one violation can exist at once, and *which* one gets rotated
 * first matters: rotating the outer one first relocates the inner
 * loop's whole span as one contiguous block (fine, its own internal
 * targets all shift together and stay consistent) -- but rotating the
 * outer one *before* the inner one has been fixed means the outer's
 * "REST" region isn't really one contiguous logical chunk yet, since
 * the still-unrotated inner loop's own increment/back-edge machinery
 * is interleaved into it; rotateonce()'s block-swap then scrambles
 * that interleaving instead of moving a clean unit. So this always
 * returns the *smallest-span* (innermost) violation found, never just
 * the first -- rotateloops() below relies on that to fix nested loops
 * inside-out, each one already-clean by the time an outer one's own
 * rotation moves it as a block.
 *
 * Returns 1 and fills *pLopen/*pTpos/*pDclose on the smallest-span
 * match found, 0 if none.
 */
static int
findrotation(Prog **live, int32 m, int32 *pLopen, int32 *pTpos, int32 *pDclose, int32 *pDpos)
{
	int32 i, j, target, lopen, dclose, dpos;
	int32 bestlopen, besttpos, bestdclose, bestdpos, bestspan;
	int found;

	found = 0;
	bestlopen = besttpos = bestdclose = bestdpos = bestspan = 0;
	for(lopen = 0; lopen < m; lopen++) {
		dclose = -1;
		for(j = 0; j < m; j++) {
			if((live[j]->as == ABR || live[j]->as == ABRIF)
			&& live[j]->to.offset == lopen && j >= lopen)
				if(dclose == -1 || j + 1 > dclose)
					dclose = j + 1;
		}
		if(dclose == -1)
			continue;
		/* claude: capture the naive (pre-extension) backward-branch
		 * position -- the fixpoint below may grow dclose to cover a
		 * nested loop's own tail, but *this* position is the one
		 * whose role actually changes under rotation (see rotateonce()
		 * / rotateonce_insert()'s comments for why the two need
		 * different treatment). */
		dpos = dclose - 1;
		/*
		 * claude: nested loops -- dclose-1 (the branch that closes
		 * *this* loop) can itself sit strictly inside some OTHER
		 * loop's own span, when that other loop is an already-
		 * rotated inner loop whose own conditional test-exit
		 * (ABRIF) is what happens to be the backward branch
		 * targeting our lopen (see the comment above this
		 * function). That ABRIF is conditional -- when not taken,
		 * control falls through into the rest of the inner loop's
		 * body/increment/back-edge, which therefore belongs inside
		 * *our* span too, or rotateonce()'s block-swap will leave
		 * it behind as unswapped "outside" material. Fix by
		 * extending dclose, to a fixpoint, to cover any such
		 * other loop's own dclose whenever dclose-1 lands inside
		 * it. Bounded by m since each extension strictly grows
		 * dclose and dclose <= m.
		 */
		for(;;) {
			int32 od, otarget, olopen, odclose, grown;

			grown = 0;
			for(od = 0; od < m; od++) {
				if(live[od]->as != ABR && live[od]->as != ABRIF)
					continue;
				otarget = live[od]->to.offset;
				if(otarget < 0 || otarget >= m || otarget == lopen)
					continue;
				if(!(otarget < dclose - 1 && dclose - 1 < od + 1))
					continue;
				/*
				 * claude: only extend into a loop *nested inside*
				 * the current candidate (its own header comes
				 * after lopen), never into one that merely
				 * *contains* lopen (an outer loop whose header
				 * sits earlier) -- otherwise an inner loop's own
				 * naive dclose wrongly balloons out to its
				 * enclosing loop's dclose, corrupting a
				 * perfectly self-contained inner rotation (seen
				 * with nested do-while: the outer do-while's own
				 * D, unrelated to the inner one, would otherwise
				 * get mistaken for something the inner loop's
				 * span needs to swallow).
				 */
				if(otarget <= lopen)
					continue;
				/* od is itself a backward branch whose target
				 * (olopen) is some other loop's header, and
				 * dclose-1 falls strictly inside [olopen, od]. */
				olopen = otarget;
				odclose = -1;
				for(j = 0; j < m; j++) {
					if((live[j]->as == ABR || live[j]->as == ABRIF)
					&& live[j]->to.offset == olopen && j >= olopen)
						if(odclose == -1 || j + 1 > odclose)
							odclose = j + 1;
				}
				if(odclose > dclose) {
					dclose = odclose;
					grown = 1;
				}
			}
			if(!grown)
				break;
		}
		for(i = 0; i < lopen; i++) {
			if(live[i]->as != ABR)
				continue;
			target = live[i]->to.offset;
			if(lopen < target && target < dclose) {
				if(!found || dclose - lopen < bestspan) {
					found = 1;
					bestlopen = lopen;
					besttpos = target;
					bestdclose = dclose;
					bestdpos = dpos;
					bestspan = dclose - lopen;
				}
			}
		}
	}
	if(found) {
		*pLopen = bestlopen;
		*pTpos = besttpos;
		*pDclose = bestdclose;
		*pDpos = bestdpos;
		return 1;
	}
	return 0;
}

/*
 * claude: the actual "loop rotation" -- a standard, named compiler
 * transformation (see docs/notes_wasm.txt for references; not
 * something any *other* arch in this codebase needs, since a real
 * arch's PC-addressed branches can jump into a loop's middle for free
 * -- this is purely a consequence of wasm's structured control flow).
 * C-level, it's `for(init;test;inc)body;` becoming roughly `init;
 * if(test){ loop{ body; inc; if(test) continue; } }` -- but
 * implemented here as a pure rearrangement of the *existing* flat
 * instructions already generated for the unrotated form, not by
 * re-emitting anything (verified equivalent by hand-tracing execution
 * order both ways -- see docs/notes_wasm.txt for the derivation):
 *
 *   before: [Lopen..T)=PREFIX  [T..Dclose-1)=REST  [Dclose-1]=D(->Lopen)
 *   after:  [Lopen..Lopen+len1)=REST (moved earlier)
 *           [Lopen+len1..Dclose-1)=PREFIX (moved later)
 *           [Dclose-1]=D, retargeted to Lopen (REST's new start)
 *
 * For a `for`, PREFIX is the increment and REST is test+body: the
 * rotated form runs test+body first (entry already skipped inc, and
 * now doesn't need to -- it just lands on REST directly), then inc,
 * then loops back to REST (retest). For a `do while`, PREFIX is the
 * test and REST is the body: entry (still skipping to T=body's old
 * position, now REST's new start) runs body then the test then loops
 * back to the body -- exactly do-while's own semantics.
 *
 * D is the one branch that does *not* follow the generic old-target-
 * to-new-target mapping (remap[]): every other branch keeps meaning
 * "go to wherever old target X's content now lives" (a `continue`
 * targeting the old Lopen still means "go run the increment", which
 * remap[Lopen] gives correctly, wherever the increment ends up). D
 * specifically is gen()'s automatic "body finished normally" edge --
 * after rotation the increment it used to jump to is reached by plain
 * fallthrough instead (it now sits immediately before D), so D's own
 * job changes to "increment just ran, go retest", i.e. always Lopen
 * (REST's fixed new start), not remap[Lopen].
 */
static void
rotateonce(Prog **live, int32 m, int32 Lopen, int32 Tpos, int32 Dclose)
{
	int32 len1, len2, i, target, idx;
	int32 *remap;
	Prog **tmp;

	len1 = Dclose - 1 - Tpos;
	len2 = Tpos - Lopen;

	remap = alloc(m * sizeof(int32));
	for(i = 0; i < m; i++)
		remap[i] = i;
	for(i = Tpos; i < Dclose - 1; i++)
		remap[i] = Lopen + (i - Tpos);
	for(i = Lopen; i < Tpos; i++)
		remap[i] = Lopen + len1 + (i - Lopen);

	tmp = alloc((len1 + len2) * sizeof(Prog*));
	for(i = 0; i < len1; i++)
		tmp[i] = live[Tpos + i];
	for(i = 0; i < len2; i++)
		tmp[len1 + i] = live[Lopen + i];
	for(i = 0; i < len1 + len2; i++)
		live[Lopen + i] = tmp[i];

	for(i = 0; i < m; i++) {
		if(i == Dclose - 1)
			continue;
		if(live[i]->as != ABR && live[i]->as != ABRIF)
			continue;
		target = live[i]->to.offset;
		if(target < 0 || target >= m)
			continue;
		idx = remap[target];
		live[i]->to.offset = idx;
	}
	live[Dclose - 1]->to.offset = Lopen;
}

/*
 * claude: nested loops can leave a loop's own "automatic loop-back"
 * edge absorbed into a *different*, inner loop's own conditional
 * test-exit branch by threadjumps()/markdead() (see findrotation()'s
 * Dpos comment: this is what makes the naive Dpos position land
 * strictly *before* the fixpoint-extended Dclose, with more of the
 * inner loop's own tail -- its body, increment, and its own D --
 * sitting in between). When that happens, there is no longer any live
 * instruction whose sole job is "the increment just ran via
 * fallthrough, now retest": the one that used to do that was jump-
 * threaded away as dead code, which is a valid optimization for a
 * flat/arbitrary-target arch (the inner loop's exit branch already
 * reaches the increment directly), but wasm's structured `loop`
 * genuinely needs an explicit instruction closing it. This path
 * synthesizes that one missing instruction (a bare ABR, hardcoded to
 * Lopen) and splices it in right after the relocated PREFIX, growing
 * the live array by one slot. Dpos itself is treated as an entirely
 * ordinary member of REST here (remapped like any other branch, not
 * hardcoded) since it is *not* adjacent to the relocated PREFIX -- the
 * newly synthesized instruction is.
 */
static Prog**
rotateonce_insert(Prog **live, int32 m, int32 Lopen, int32 Tpos, int32 Dclose, int32 *pm)
{
	int32 restlen, prefixlen, tailidx, i, target;
	int32 *remap;
	Prog **newlive;
	Prog *newd;

	restlen = Dclose - Tpos;
	prefixlen = Tpos - Lopen;
	tailidx = Lopen + restlen + prefixlen;

	remap = alloc(m * sizeof(int32));
	for(i = 0; i < m; i++)
		remap[i] = i;
	for(i = Tpos; i < Dclose; i++)
		remap[i] = Lopen + (i - Tpos);
	for(i = Lopen; i < Tpos; i++)
		remap[i] = Lopen + restlen + (i - Lopen);
	for(i = Dclose; i < m; i++)
		remap[i] = tailidx + 1 + (i - Dclose);

	newlive = alloc((m + 1) * sizeof(Prog*));
	for(i = 0; i < Lopen; i++)
		newlive[i] = live[i];
	for(i = 0; i < restlen; i++)
		newlive[Lopen + i] = live[Tpos + i];
	for(i = 0; i < prefixlen; i++)
		newlive[Lopen + restlen + i] = live[Lopen + i];

	newd = newprog(ABR);
	newd->to.offset = Lopen;
	newlive[tailidx] = newd;

	for(i = Dclose; i < m; i++)
		newlive[tailidx + 1 + (i - Dclose)] = live[i];

	for(i = 0; i < m; i++) {
		if(newlive[remap[i]]->as != ABR && newlive[remap[i]]->as != ABRIF)
			continue;
		target = newlive[remap[i]]->to.offset;
		if(target < 0 || target >= m)
			continue;
		newlive[remap[i]]->to.offset = remap[target];
	}

	*pm = m + 1;
	return newlive;
}

/*
 * claude: bounded by m tries -- each successful rotation strictly
 * shrinks how many for/do-while shapes remain (it doesn't introduce
 * new ones: see rotateonce()'s own comment; rotateonce_insert() grows
 * the array by one Prog, not by one violation), so this should always
 * terminate well under that; the bound is just a safety net against
 * a bug turning this into an infinite loop instead of a diag(). May
 * return a *different* live array than it was given (rotateonce_insert()
 * reallocates), always updating *pm to match whatever it returns.
 */
static Prog**
rotateloops(Prog **live, int32 *pm)
{
	int32 lopen, tpos, dclose, dpos, m;
	int32 tries;

	m = *pm;
	for(tries = 0; tries < m + 1; tries++) {
		if(!findrotation(live, m, &lopen, &tpos, &dclose, &dpos)) {
			*pm = m;
			return live;
		}
		if(dpos == dclose - 1)
			rotateonce(live, m, lopen, tpos, dclose);
		else
			live = rotateonce_insert(live, m, lopen, tpos, dclose, &m);
	}
	diag(Z, "ec: loop rotation did not converge");
	*pm = m;
	return live;
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
 * that loop. rotateloops() (run much earlier, right after threading)
 * already rewrites the one real case that produces this -- a C
 * `for`/`do while`'s entry jump -- into something block/loop nesting
 * *can* express, so nothing should reach this check in practice.
 * Kept as a safety net (clear diagnostic instead of silently emitting
 * a module that fails wasm validation) in case some shape neither
 * rotateloops() nor cck's own frontend diag()s slips through.
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

	live = rotateloops(live, &m);

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
