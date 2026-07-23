/*
 * ec/sgen.c -- the addressability/complexity annotation pass (xcom).
 * Statement-level control flow (if/for/while/return/goto/switch) and
 * the per-function ATEXT setup both live entirely in the shared
 * compilers/cck/pgen.c's codgen(), not here -- see docs/notes_wasm.txt
 * and txt.c's gpseudo() (which is what codgen() actually calls to
 * emit ATEXT, and where ASIGNATURE gets emitted alongside it).
 */
#include "gc.h"

/*
 * claude: no register liveness to mark at all (see gc.h's comment on
 * stackresult) -- a true no-op, unlike ic/sgen.c's ANOP-with-D_REG
 * markers.
 */
void
noretval(int n)
{
	USED(n);
}

/*
 * claude: modeled on ic/sgen.c's xcom(), minus the strength-reduction
 * transforms (mul/div-by-power-of-2 -> shift): those are a pure
 * optimization ic needs because a real multiply instruction is
 * costlier there, whereas wasm's i32.mul/i64.mul are single opcodes
 * same as a shift, so there's nothing to gain by rewriting the tree.
 * `complex` (the Sethi-Ullman register-pressure estimate) is set for
 * structural compatibility but unused by ec's cgen(): a stack machine
 * has no register pressure to minimize, see cgen.c's file comment.
 */
void
xcom(Node *n)
{
	Node *l, *r;

	if(n == Z)
		return;
	l = n->left;
	r = n->right;
	n->addable = 0;
	n->complex = 0;
	switch(n->op) {
	case OCONST:
		n->addable = 20;
		return;

	case OREGISTER:
		n->addable = 11;
		return;

	case ONAME:
		n->addable = 10;
		return;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}
	if(n->addable >= 10)
		return;

	if(l != Z)
		n->complex = l->complex;
	if(r != Z) {
		if(r->complex == n->complex)
			n->complex = r->complex+1;
		else if(r->complex > n->complex)
			n->complex = r->complex;
	}
	if(n->complex == 0)
		n->complex++;
	if(n->op == OFUNC)
		n->complex = FNX;
}
