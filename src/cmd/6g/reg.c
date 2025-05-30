// Derived from Inferno utils/6c/reg.c
// http://code.google.com/p/inferno-os/source/browse/utils/6c/reg.c
//
//	Copyright © 1994-1999 Lucent Technologies Inc.  All rights reserved.
//	Portions Copyright © 1995-1997 C H Forsyth (forsyth@terzarima.net)
//	Portions Copyright © 1997-1999 Vita Nuova Limited
//	Portions Copyright © 2000-2007 Vita Nuova Holdings Limited (www.vitanuova.com)
//	Portions Copyright © 2004,2006 Bruce Ellis
//	Portions Copyright © 2005-2007 C H Forsyth (forsyth@terzarima.net)
//	Revisions Copyright © 2000-2007 Lucent Technologies Inc. and others
//	Portions Copyright © 2009 The Go Authors.  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "gg.h"
#undef	EXTERN
#define	EXTERN
#include "opt.h"

#define	P2R(p)	(Reg*)(p->reg)

static	int	first	= 1;

Reg*
rega(void)
{
	Reg *r;

	r = freer;
	if(r == R) {
		r = mal(sizeof(*r));
	} else
		freer = r->link;

	*r = zreg;
	return r;
}

int
rcmp(const void *a1, const void *a2)
{
	Rgn *p1, *p2;
	int c1, c2;

	p1 = (Rgn*)a1;
	p2 = (Rgn*)a2;
	c1 = p2->cost;
	c2 = p1->cost;
	if(c1 -= c2)
		return c1;
	return p2->varno - p1->varno;
}

static void
setoutvar(void)
{
	Type *t;
	Node *n;
	Addr a;
	Iter save;
	Bits bit;
	int z;

	t = structfirst(&save, getoutarg(curfn->type));
	while(t != T) {
		n = nodarg(t, 1);
		a = zprog.from;
		naddr(n, &a, 0);
		bit = mkvar(R, &a);
		for(z=0; z<BITS; z++)
			ovar.b[z] |= bit.b[z];
		t = structnext(&save);
	}
//if(bany(b))
//print("ovars = %Q\n", &ovar);
}

static void
setaddrs(Bits bit)
{
	int i, n;
	Var *v;
	Sym *s;

	while(bany(&bit)) {
		// convert each bit to a variable
		i = bnum(bit);
		s = var[i].sym;
		n = var[i].name;
		bit.b[i/32] &= ~(1L<<(i%32));

		// disable all pieces of that variable
		for(i=0; i<nvar; i++) {
			v = var+i;
			if(v->sym == s && v->name == n)
				v->addr = 2;
		}
	}
}

void
regopt(Prog *firstp)
{
	Reg *r, *r1;
	Prog *p;
	int i, z, nr;
	uint32 vreg;
	Bits bit;

	if(first) {
		fmtinstall('Q', Qconv);
		exregoffset = D_R13;	// R14,R15 are external
		first = 0;
	}

	// count instructions
	nr = 0;
	for(p=firstp; p!=P; p=p->link)
		nr++;
	// if too big dont bother
	if(nr >= 10000) {
//		print("********** %S is too big (%d)\n", curfn->nname->sym, nr);
		return;
	}

	r1 = R;
	firstr = R;
	lastr = R;
	nvar = 0;
	regbits = RtoB(D_SP);
	for(z=0; z<BITS; z++) {
		externs.b[z] = 0;
		params.b[z] = 0;
		consts.b[z] = 0;
		addrs.b[z] = 0;
		ovar.b[z] = 0;
	}

	// build list of return variables
	setoutvar();

	/*
	 * pass 1
	 * build aux data structure
	 * allocate pcs
	 * find use and set of variables
	 */
	nr = 0;
	for(p=firstp; p!=P; p=p->link) {
		switch(p->as) {
		case ADATA:
		case AGLOBL:
		case ANAME:
		case ASIGNAME:
			continue;
		}
		r = rega();
		nr++;
		if(firstr == R) {
			firstr = r;
			lastr = r;
		} else {
			lastr->link = r;
			r->p1 = lastr;
			lastr->s1 = r;
			lastr = r;
		}
		r->prog = p;
		p->reg = r;

		r1 = r->p1;
		if(r1 != R) {
			switch(r1->prog->as) {
			case ARET:
			case AJMP:
			case AIRETL:
			case AIRETQ:
				r->p1 = R;
				r1->s1 = R;
			}
		}

		bit = mkvar(r, &p->from);
		if(bany(&bit))
		switch(p->as) {
		/*
		 * funny
		 */
		case ALEAL:
		case ALEAQ:
			setaddrs(bit);
			break;

		/*
		 * left side read
		 */
		default:
			for(z=0; z<BITS; z++)
				r->use1.b[z] |= bit.b[z];
			break;

		/*
		 * left side read+write
		 */
		case AXCHGB:
		case AXCHGW:
		case AXCHGL:
		case AXCHGQ:
			for(z=0; z<BITS; z++) {
				r->use1.b[z] |= bit.b[z];
				r->set.b[z] |= bit.b[z];
			}
			break;
		}

		bit = mkvar(r, &p->to);
		if(bany(&bit))
		switch(p->as) {
		default:
			yyerror("reg: unknown op: %A", p->as);
			break;

		/*
		 * right side read
		 */
		case ACMPB:
		case ACMPL:
		case ACMPQ:
		case ACMPW:
		case ACOMISS:
		case ACOMISD:
		case AUCOMISS:
		case AUCOMISD:
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];
			break;

		/*
		 * right side write
		 */
		case ANOP:
		case AMOVL:
		case AMOVQ:
		case AMOVB:
		case AMOVW:
		case AMOVBLSX:
		case AMOVBLZX:
		case AMOVBQSX:
		case AMOVBQZX:
		case AMOVLQSX:
		case AMOVLQZX:
		case AMOVWLSX:
		case AMOVWLZX:
		case AMOVWQSX:
		case AMOVWQZX:

		case AMOVSS:
		case AMOVSD:
		case ACVTSD2SL:
		case ACVTSD2SQ:
		case ACVTSD2SS:
		case ACVTSL2SD:
		case ACVTSL2SS:
		case ACVTSQ2SD:
		case ACVTSQ2SS:
		case ACVTSS2SD:
		case ACVTSS2SL:
		case ACVTSS2SQ:
		case ACVTTSD2SL:
		case ACVTTSD2SQ:
		case ACVTTSS2SL:
		case ACVTTSS2SQ:
			for(z=0; z<BITS; z++)
				r->set.b[z] |= bit.b[z];
			break;

		/*
		 * right side read+write
		 */
		case AINCB:
		case AINCL:
		case AINCQ:
		case AINCW:
		case ADECB:
		case ADECL:
		case ADECQ:
		case ADECW:

		case AADDB:
		case AADDL:
		case AADDQ:
		case AADDW:
		case AANDB:
		case AANDL:
		case AANDQ:
		case AANDW:
		case ASUBB:
		case ASUBL:
		case ASUBQ:
		case ASUBW:
		case AORB:
		case AORL:
		case AORQ:
		case AORW:
		case AXORB:
		case AXORL:
		case AXORQ:
		case AXORW:
		case ASALB:
		case ASALL:
		case ASALQ:
		case ASALW:
		case ASARB:
		case ASARL:
		case ASARQ:
		case ASARW:
		case ARCLB:
		case ARCLL:
		case ARCLQ:
		case ARCLW:
		case ARCRB:
		case ARCRL:
		case ARCRQ:
		case ARCRW:
		case AROLB:
		case AROLL:
		case AROLQ:
		case AROLW:
		case ARORB:
		case ARORL:
		case ARORQ:
		case ARORW:
		case ASHLB:
		case ASHLL:
		case ASHLQ:
		case ASHLW:
		case ASHRB:
		case ASHRL:
		case ASHRQ:
		case ASHRW:
		case AIMULL:
		case AIMULQ:
		case AIMULW:
		case ANEGL:
		case ANEGQ:
		case ANOTL:
		case ANOTQ:
		case AADCL:
		case AADCQ:
		case ASBBL:
		case ASBBQ:

		case AXCHGB:
		case AXCHGW:
		case AXCHGL:
		case AXCHGQ:

		case AADDSD:
		case AADDSS:
		case ACMPSD:
		case ACMPSS:
		case ADIVSD:
		case ADIVSS:
		case AMAXSD:
		case AMAXSS:
		case AMINSD:
		case AMINSS:
		case AMULSD:
		case AMULSS:
		case ARCPSS:
		case ARSQRTSS:
		case ASQRTSD:
		case ASQRTSS:
		case ASUBSD:
		case ASUBSS:
		case AXORPD:
			for(z=0; z<BITS; z++) {
				r->set.b[z] |= bit.b[z];
				r->use2.b[z] |= bit.b[z];
			}
			break;

		/*
		 * funny
		 */
		case ACALL:
			setaddrs(bit);
			break;
		}

		switch(p->as) {
		case AIMULL:
		case AIMULQ:
		case AIMULW:
			if(p->to.type != D_NONE)
				break;

		case AIDIVB:
		case AIDIVL:
		case AIDIVQ:
		case AIDIVW:
		case AIMULB:
		case ADIVB:
		case ADIVL:
		case ADIVQ:
		case ADIVW:
		case AMULB:
		case AMULL:
		case AMULQ:
		case AMULW:

		case ACWD:
		case ACDQ:
		case ACQO:
			r->regu |= RtoB(D_AX) | RtoB(D_DX);
			break;

		case AREP:
		case AREPN:
		case ALOOP:
		case ALOOPEQ:
		case ALOOPNE:
			r->regu |= RtoB(D_CX);
			break;

		case AMOVSB:
		case AMOVSL:
		case AMOVSQ:
		case AMOVSW:
		case ACMPSB:
		case ACMPSL:
		case ACMPSQ:
		case ACMPSW:
			r->regu |= RtoB(D_SI) | RtoB(D_DI);
			break;

		case ASTOSB:
		case ASTOSL:
		case ASTOSQ:
		case ASTOSW:
		case ASCASB:
		case ASCASL:
		case ASCASQ:
		case ASCASW:
			r->regu |= RtoB(D_AX) | RtoB(D_DI);
			break;

		case AINSB:
		case AINSL:
		case AINSW:
		case AOUTSB:
		case AOUTSL:
		case AOUTSW:
			r->regu |= RtoB(D_DI) | RtoB(D_DX);
			break;
		}
	}
	if(firstr == R)
		return;

	for(i=0; i<nvar; i++) {
		Var *v = var+i;
		if(v->addr) {
			bit = blsh(i);
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
		}

//		print("bit=%2d addr=%d et=%-6E w=%-2d s=%S + %lld\n",
//			i, v->addr, v->etype, v->width, v->sym, v->offset);
	}

	if(debug['R'] && debug['v'])
		dumpit("pass1", firstr);

	/*
	 * pass 2
	 * turn branch references to pointers
	 * build back pointers
	 */
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		if(p->to.type == D_BRANCH) {
			if(p->to.branch == P)
				fatal("pnil %P", p);
			r1 = p->to.branch->reg;
			if(r1 == R)
				fatal("rnil %P", p);
			if(r1 == r) {
				//fatal("ref to self %P", p);
				continue;
			}
			r->s2 = r1;
			r->p2link = r1->p2;
			r1->p2 = r;
		}
	}

	if(debug['R'] && debug['v'])
		dumpit("pass2", firstr);

	/*
	 * pass 2.5
	 * find looping structure
	 */
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	change = 0;
	loopit(firstr, nr);

	if(debug['R'] && debug['v'])
		dumpit("pass2.5", firstr);

	/*
	 * pass 3
	 * iterate propagating usage
	 * 	back until flow graph is complete
	 */
loop1:
	change = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	for(r = firstr; r != R; r = r->link)
		if(r->prog->as == ARET)
			prop(r, zbits, zbits);
loop11:
	/* pick up unreachable code */
	i = 0;
	for(r = firstr; r != R; r = r1) {
		r1 = r->link;
		if(r1 && r1->active && !r->active) {
			prop(r, zbits, zbits);
			i = 1;
		}
	}
	if(i)
		goto loop11;
	if(change)
		goto loop1;

	if(debug['R'] && debug['v'])
		dumpit("pass3", firstr);

	/*
	 * pass 4
	 * iterate propagating register/variable synchrony
	 * 	forward until graph is complete
	 */
loop2:
	change = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	synch(firstr, zbits);
	if(change)
		goto loop2;

	if(debug['R'] && debug['v'])
		dumpit("pass4", firstr);

	/*
	 * pass 5
	 * isolate regions
	 * calculate costs (paint1)
	 */
	r = firstr;
	if(r) {
		for(z=0; z<BITS; z++)
			bit.b[z] = (r->refahead.b[z] | r->calahead.b[z]) &
			  ~(externs.b[z] | params.b[z] | addrs.b[z] | consts.b[z]);
		if(bany(&bit) && !r->refset) {
			// should never happen - all variables are preset
			if(debug['w'])
				print("%L: used and not set: %Q\n", r->prog->lineno, bit);
			r->refset = 1;
		}
	}
	for(r = firstr; r != R; r = r->link)
		r->act = zbits;
	rgp = region;
	nregion = 0;
	for(r = firstr; r != R; r = r->link) {
		for(z=0; z<BITS; z++)
			bit.b[z] = r->set.b[z] &
			  ~(r->refahead.b[z] | r->calahead.b[z] | addrs.b[z]);
		if(bany(&bit) && !r->refset) {
			if(debug['w'])
				print("%L: set and not used: %Q\n", r->prog->lineno, bit);
			r->refset = 1;
			excise(r);
		}
		for(z=0; z<BITS; z++)
			bit.b[z] = LOAD(r) & ~(r->act.b[z] | addrs.b[z]);
		while(bany(&bit)) {
			i = bnum(bit);
			rgp->enter = r;
			rgp->varno = i;
			change = 0;
			paint1(r, i);
			bit.b[i/32] &= ~(1L<<(i%32));
			if(change <= 0)
				continue;
			rgp->cost = change;
			nregion++;
			if(nregion >= NRGN) {
				if(debug['R'] && debug['v'])
					print("too many regions\n");
				goto brk;
			}
			rgp++;
		}
	}
brk:
	qsort(region, nregion, sizeof(region[0]), rcmp);

	/*
	 * pass 6
	 * determine used registers (paint2)
	 * replace code (paint3)
	 */
	rgp = region;
	for(i=0; i<nregion; i++) {
		bit = blsh(rgp->varno);
		vreg = paint2(rgp->enter, rgp->varno);
		vreg = allreg(vreg, rgp);
		if(rgp->regno != 0)
			paint3(rgp->enter, rgp->varno, vreg, rgp->regno);
		rgp++;
	}

	if(debug['R'] && debug['v'])
		dumpit("pass6", firstr);

	/*
	 * pass 7
	 * peep-hole on basic block
	 */
	if(!debug['R'] || debug['P']) {
		peep();
	}

	/*
	 * eliminate nops
	 * free aux structures
	 */
	for(p=firstp; p!=P; p=p->link) {
		while(p->link != P && p->link->as == ANOP)
			p->link = p->link->link;
		if(p->to.type == D_BRANCH)
			while(p->to.branch != P && p->to.branch->as == ANOP)
				p->to.branch = p->to.branch->link;
	}

	if(r1 != R) {
		r1->link = freer;
		freer = firstr;
	}

	if(debug['R']) {
		if(ostats.ncvtreg ||
		   ostats.nspill ||
		   ostats.nreload ||
		   ostats.ndelmov ||
		   ostats.nvar ||
		   ostats.naddr ||
		   0)
			print("\nstats\n");

		if(ostats.ncvtreg)
			print("	%4d cvtreg\n", ostats.ncvtreg);
		if(ostats.nspill)
			print("	%4d spill\n", ostats.nspill);
		if(ostats.nreload)
			print("	%4d reload\n", ostats.nreload);
		if(ostats.ndelmov)
			print("	%4d delmov\n", ostats.ndelmov);
		if(ostats.nvar)
			print("	%4d delmov\n", ostats.nvar);
		if(ostats.naddr)
			print("	%4d delmov\n", ostats.naddr);

		memset(&ostats, 0, sizeof(ostats));
	}
}

/*
 * add mov b,rn
 * just after r
 */
void
addmove(Reg *r, int bn, int rn, int f)
{
	Prog *p, *p1;
	Adr *a;
	Var *v;

	p1 = mal(sizeof(*p1));
	clearp(p1);
	p1->loc = 9999;

	p = r->prog;
	p1->link = p->link;
	p->link = p1;
	p1->lineno = p->lineno;

	v = var + bn;

	a = &p1->to;
	a->sym = v->sym;
	a->offset = v->offset;
	a->etype = v->etype;
	a->type = v->name;
	a->gotype = v->gotype;

	// need to clean this up with wptr and
	// some of the defaults
	p1->as = AMOVL;
	switch(v->etype) {
	default:
		fatal("unknown type\n");
	case TINT8:
	case TUINT8:
	case TBOOL:
		p1->as = AMOVB;
		break;
	case TINT16:
	case TUINT16:
		p1->as = AMOVW;
		break;
	case TINT64:
	case TUINT64:
	case TUINTPTR:
	case TPTR64:
		p1->as = AMOVQ;
		break;
	case TFLOAT:
	case TFLOAT32:
		p1->as = AMOVSS;
		break;
	case TFLOAT64:
		p1->as = AMOVSD;
		break;
	case TINT:
	case TUINT:
	case TINT32:
	case TUINT32:
	case TPTR32:
		break;
	}

	p1->from.type = rn;
	if(!f) {
		p1->from = *a;
		*a = zprog.from;
		a->type = rn;
		if(v->etype == TUINT8)
			p1->as = AMOVB;
		if(v->etype == TUINT16)
			p1->as = AMOVW;
	}
	if(debug['R'] && debug['v'])
		print("%P ===add=== %P\n", p, p1);
	ostats.nspill++;
}

uint32
doregbits(int r)
{
	uint32 b;

	b = 0;
	if(r >= D_INDIR)
		r -= D_INDIR;
	if(r >= D_AX && r <= D_R15)
		b |= RtoB(r);
	else
	if(r >= D_AL && r <= D_R15B)
		b |= RtoB(r-D_AL+D_AX);
	else
	if(r >= D_AH && r <= D_BH)
		b |= RtoB(r-D_AH+D_AX);
	else
	if(r >= D_X0 && r <= D_X0+15)
		b |= FtoB(r);
	return b;
}

static int
overlap(int32 o1, int w1, int32 o2, int w2)
{
	int32 t1, t2;

	t1 = o1+w1;
	t2 = o2+w2;

	if(!(t1 > o2 && t2 > o1))
		return 0;

	return 1;
}

Bits
mkvar(Reg *r, Adr *a)
{
	Var *v;
	int i, t, n, et, z, w, flag;
	int32 o;
	Bits bit;
	Sym *s;

	/*
	 * mark registers used
	 */
	t = a->type;
	if(t == D_NONE)
		goto none;

	if(r != R) {
		r->regu |= doregbits(t);
		r->regu |= doregbits(a->index);
	}

	switch(t) {
	default:
		goto none;

	case D_ADDR:
		a->type = a->index;
		bit = mkvar(r, a);
		setaddrs(bit);
		a->type = t;
		ostats.naddr++;
		goto none;

	case D_EXTERN:
	case D_STATIC:
	case D_PARAM:
	case D_AUTO:
		n = t;
		break;
	}
	s = a->sym;
	if(s == S)
		goto none;
	if(s->name[0] == '.')
		goto none;
	et = a->etype;
	o = a->offset;
	w = a->width;

	flag = 0;
	for(i=0; i<nvar; i++) {
		v = var+i;
		if(v->sym == s && v->name == n) {
			if(v->offset == o)
			if(v->etype == et)
			if(v->width == w)
				return blsh(i);

			// if they overlaps, disable both
			if(overlap(v->offset, v->width, o, w)) {
				v->addr = 1;
				flag = 1;
			}
		}
	}
	if(a->pun)
		flag = 1;

	switch(et) {
	case 0:
	case TFUNC:
		goto none;
	}

	if(nvar >= NVAR) {
		if(debug['w'] > 1 && s)
			fatal("variable not optimized: %D", a);
		goto none;
	}

	i = nvar;
	nvar++;
	v = var+i;
	v->sym = s;
	v->offset = o;
	v->name = n;
	v->gotype = a->gotype;
	v->etype = et;
	v->width = w;
	v->addr = flag;		// funny punning

	if(debug['R'])
		print("bit=%2d et=%2d w=%d %S %D\n", i, et, w, s, a);
	ostats.nvar++;

	bit = blsh(i);
	if(n == D_EXTERN || n == D_STATIC)
		for(z=0; z<BITS; z++)
			externs.b[z] |= bit.b[z];
	if(n == D_PARAM)
		for(z=0; z<BITS; z++)
			params.b[z] |= bit.b[z];

	return bit;

none:
	return zbits;
}

void
prop(Reg *r, Bits ref, Bits cal)
{
	Reg *r1, *r2;
	int z;

	for(r1 = r; r1 != R; r1 = r1->p1) {
		for(z=0; z<BITS; z++) {
			ref.b[z] |= r1->refahead.b[z];
			if(ref.b[z] != r1->refahead.b[z]) {
				r1->refahead.b[z] = ref.b[z];
				change++;
			}
			cal.b[z] |= r1->calahead.b[z];
			if(cal.b[z] != r1->calahead.b[z]) {
				r1->calahead.b[z] = cal.b[z];
				change++;
			}
		}
		switch(r1->prog->as) {
		case ACALL:
			if(noreturn(r1->prog))
				break;
			for(z=0; z<BITS; z++) {
				cal.b[z] |= ref.b[z] | externs.b[z];
				ref.b[z] = 0;
			}
			break;

		case ATEXT:
			for(z=0; z<BITS; z++) {
				cal.b[z] = 0;
				ref.b[z] = 0;
			}
			break;

		case ARET:
			for(z=0; z<BITS; z++) {
				cal.b[z] = externs.b[z] | ovar.b[z];
				ref.b[z] = 0;
			}
			break;
		}
		for(z=0; z<BITS; z++) {
			ref.b[z] = (ref.b[z] & ~r1->set.b[z]) |
				r1->use1.b[z] | r1->use2.b[z];
			cal.b[z] &= ~(r1->set.b[z] | r1->use1.b[z] | r1->use2.b[z]);
			r1->refbehind.b[z] = ref.b[z];
			r1->calbehind.b[z] = cal.b[z];
		}
		if(r1->active)
			break;
		r1->active = 1;
	}
	for(; r != r1; r = r->p1)
		for(r2 = r->p2; r2 != R; r2 = r2->p2link)
			prop(r2, r->refbehind, r->calbehind);
}

/*
 * find looping structure
 *
 * 1) find reverse postordering
 * 2) find approximate dominators,
 *	the actual dominators if the flow graph is reducible
 *	otherwise, dominators plus some other non-dominators.
 *	See Matthew S. Hecht and Jeffrey D. Ullman,
 *	"Analysis of a Simple Algorithm for Global Data Flow Problems",
 *	Conf.  Record of ACM Symp. on Principles of Prog. Langs, Boston, Massachusetts,
 *	Oct. 1-3, 1973, pp.  207-217.
 * 3) find all nodes with a predecessor dominated by the current node.
 *	such a node is a loop head.
 *	recursively, all preds with a greater rpo number are in the loop
 */
int32
postorder(Reg *r, Reg **rpo2r, int32 n)
{
	Reg *r1;

	r->rpo = 1;
	r1 = r->s1;
	if(r1 && !r1->rpo)
		n = postorder(r1, rpo2r, n);
	r1 = r->s2;
	if(r1 && !r1->rpo)
		n = postorder(r1, rpo2r, n);
	rpo2r[n] = r;
	n++;
	return n;
}

int32
rpolca(int32 *idom, int32 rpo1, int32 rpo2)
{
	int32 t;

	if(rpo1 == -1)
		return rpo2;
	while(rpo1 != rpo2){
		if(rpo1 > rpo2){
			t = rpo2;
			rpo2 = rpo1;
			rpo1 = t;
		}
		while(rpo1 < rpo2){
			t = idom[rpo2];
			if(t >= rpo2)
				fatal("bad idom");
			rpo2 = t;
		}
	}
	return rpo1;
}

int
doms(int32 *idom, int32 r, int32 s)
{
	while(s > r)
		s = idom[s];
	return s == r;
}

int
loophead(int32 *idom, Reg *r)
{
	int32 src;

	src = r->rpo;
	if(r->p1 != R && doms(idom, src, r->p1->rpo))
		return 1;
	for(r = r->p2; r != R; r = r->p2link)
		if(doms(idom, src, r->rpo))
			return 1;
	return 0;
}

void
loopmark(Reg **rpo2r, int32 head, Reg *r)
{
	if(r->rpo < head || r->active == head)
		return;
	r->active = head;
	r->loop += LOOP;
	if(r->p1 != R)
		loopmark(rpo2r, head, r->p1);
	for(r = r->p2; r != R; r = r->p2link)
		loopmark(rpo2r, head, r);
}

void
loopit(Reg *r, int32 nr)
{
	Reg *r1;
	int32 i, d, me;

	if(nr > maxnr) {
		rpo2r = mal(nr * sizeof(Reg*));
		idom = mal(nr * sizeof(int32));
		maxnr = nr;
	}

	d = postorder(r, rpo2r, 0);
	if(d > nr)
		fatal("too many reg nodes %d %d", d, nr);
	nr = d;
	for(i = 0; i < nr / 2; i++) {
		r1 = rpo2r[i];
		rpo2r[i] = rpo2r[nr - 1 - i];
		rpo2r[nr - 1 - i] = r1;
	}
	for(i = 0; i < nr; i++)
		rpo2r[i]->rpo = i;

	idom[0] = 0;
	for(i = 0; i < nr; i++) {
		r1 = rpo2r[i];
		me = r1->rpo;
		d = -1;
		if(r1->p1 != R && r1->p1->rpo < me)
			d = r1->p1->rpo;
		for(r1 = r1->p2; r1 != nil; r1 = r1->p2link)
			if(r1->rpo < me)
				d = rpolca(idom, d, r1->rpo);
		idom[i] = d;
	}

	for(i = 0; i < nr; i++) {
		r1 = rpo2r[i];
		r1->loop++;
		if(r1->p2 != R && loophead(idom, r1))
			loopmark(rpo2r, i, r1);
	}
}

void
synch(Reg *r, Bits dif)
{
	Reg *r1;
	int z;

	for(r1 = r; r1 != R; r1 = r1->s1) {
		for(z=0; z<BITS; z++) {
			dif.b[z] = (dif.b[z] &
				~(~r1->refbehind.b[z] & r1->refahead.b[z])) |
					r1->set.b[z] | r1->regdiff.b[z];
			if(dif.b[z] != r1->regdiff.b[z]) {
				r1->regdiff.b[z] = dif.b[z];
				change++;
			}
		}
		if(r1->active)
			break;
		r1->active = 1;
		for(z=0; z<BITS; z++)
			dif.b[z] &= ~(~r1->calbehind.b[z] & r1->calahead.b[z]);
		if(r1->s2 != R)
			synch(r1->s2, dif);
	}
}

uint32
allreg(uint32 b, Rgn *r)
{
	Var *v;
	int i;

	v = var + r->varno;
	r->regno = 0;
	switch(v->etype) {

	default:
		fatal("unknown etype %d/%E", bitno(b), v->etype);
		break;

	case TINT8:
	case TUINT8:
	case TINT16:
	case TUINT16:
	case TINT32:
	case TUINT32:
	case TINT64:
	case TUINT64:
	case TINT:
	case TUINT:
	case TUINTPTR:
	case TBOOL:
	case TPTR32:
	case TPTR64:
		i = BtoR(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return RtoB(i);
		}
		break;

	case TFLOAT32:
	case TFLOAT64:
	case TFLOAT:
		i = BtoF(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return FtoB(i);
		}
		break;
	}
	return 0;
}

void
paint1(Reg *r, int bn)
{
	Reg *r1;
	Prog *p;
	int z;
	uint32 bb;

	z = bn/32;
	bb = 1L<<(bn%32);
	if(r->act.b[z] & bb)
		return;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(r1->act.b[z] & bb)
			break;
		r = r1;
	}

	if(LOAD(r) & ~(r->set.b[z]&~(r->use1.b[z]|r->use2.b[z])) & bb) {
		change -= CLOAD * r->loop;
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			change += CREF * r->loop;
		}

		if((r->use2.b[z]|r->set.b[z]) & bb) {
			change += CREF * r->loop;
		}

		if(STORE(r) & r->regdiff.b[z] & bb) {
			change -= CLOAD * r->loop;
		}

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
				if(r1->refahead.b[z] & bb)
					paint1(r1, bn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				paint1(r1, bn);
		r = r->s1;
		if(r == R)
			break;
		if(r->act.b[z] & bb)
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
}

uint32
regset(Reg *r, uint32 bb)
{
	uint32 b, set;
	Adr v;
	int c;

	set = 0;
	v = zprog.from;
	while(b = bb & ~(bb-1)) {
		v.type = b & 0xFFFF? BtoR(b): BtoF(b);
		if(v.type == 0)
			fatal("zero v.type for %#ux", b);
		c = copyu(r->prog, &v, A);
		if(c == 3)
			set |= b;
		bb &= ~b;
	}
	return set;
}

uint32
reguse(Reg *r, uint32 bb)
{
	uint32 b, set;
	Adr v;
	int c;

	set = 0;
	v = zprog.from;
	while(b = bb & ~(bb-1)) {
		v.type = b & 0xFFFF? BtoR(b): BtoF(b);
		c = copyu(r->prog, &v, A);
		if(c == 1 || c == 2 || c == 4)
			set |= b;
		bb &= ~b;
	}
	return set;
}

uint32
paint2(Reg *r, int bn)
{
	Reg *r1;
	int z;
	uint32 bb, vreg, x;

	z = bn/32;
	bb = 1L << (bn%32);
	vreg = regbits;
	if(!(r->act.b[z] & bb))
		return vreg;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(!(r1->act.b[z] & bb))
			break;
		r = r1;
	}
	for(;;) {
		r->act.b[z] &= ~bb;

		vreg |= r->regu;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
				if(r1->refahead.b[z] & bb)
					vreg |= paint2(r1, bn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				vreg |= paint2(r1, bn);
		r = r->s1;
		if(r == R)
			break;
		if(!(r->act.b[z] & bb))
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}

	bb = vreg;
	for(; r; r=r->s1) {
		x = r->regu & ~bb;
		if(x) {
			vreg |= reguse(r, x);
			bb |= regset(r, x);
		}
	}
	return vreg;
}

void
paint3(Reg *r, int bn, int32 rb, int rn)
{
	Reg *r1;
	Prog *p;
	int z;
	uint32 bb;

	z = bn/32;
	bb = 1L << (bn%32);
	if(r->act.b[z] & bb)
		return;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(r1->act.b[z] & bb)
			break;
		r = r1;
	}

	if(LOAD(r) & ~(r->set.b[z] & ~(r->use1.b[z]|r->use2.b[z])) & bb)
		addmove(r, bn, rn, 0);
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			if(debug['R'] && debug['v'])
				print("%P", p);
			addreg(&p->from, rn);
			if(debug['R'] && debug['v'])
				print(" ===change== %P\n", p);
		}
		if((r->use2.b[z]|r->set.b[z]) & bb) {
			if(debug['R'] && debug['v'])
				print("%P", p);
			addreg(&p->to, rn);
			if(debug['R'] && debug['v'])
				print(" ===change== %P\n", p);
		}

		if(STORE(r) & r->regdiff.b[z] & bb)
			addmove(r, bn, rn, 1);
		r->regu |= rb;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
				if(r1->refahead.b[z] & bb)
					paint3(r1, bn, rb, rn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				paint3(r1, bn, rb, rn);
		r = r->s1;
		if(r == R)
			break;
		if(r->act.b[z] & bb)
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
}

void
addreg(Adr *a, int rn)
{

	a->sym = 0;
	a->offset = 0;
	a->type = rn;

	ostats.ncvtreg++;
}

int32
RtoB(int r)
{

	if(r < D_AX || r > D_R15)
		return 0;
	return 1L << (r-D_AX);
}

int
BtoR(int32 b)
{
	b &= 0x3fffL;		// no R14 or R15
	if(b == 0)
		return 0;
	return bitno(b) + D_AX;
}

/*
 *	bit	reg
 *	16	X5 (FREGMIN)
 *	...
 *	26	X15 (FREGEXT)
 */
int32
FtoB(int f)
{
	if(f < FREGMIN || f > FREGEXT)
		return 0;
	return 1L << (f - FREGMIN + 16);
}

int
BtoF(int32 b)
{

	b &= 0xFF0000L;
	if(b == 0)
		return 0;
	return bitno(b) - 16 + FREGMIN;
}

void
dumpone(Reg *r)
{
	int z;
	Bits bit;

	print("%d:%P", r->loop, r->prog);
	for(z=0; z<BITS; z++)
		bit.b[z] =
			r->set.b[z] |
			r->use1.b[z] |
			r->use2.b[z] |
			r->refbehind.b[z] |
			r->refahead.b[z] |
			r->calbehind.b[z] |
			r->calahead.b[z] |
			r->regdiff.b[z] |
			r->act.b[z] |
				0;
	if(bany(&bit)) {
		print("\t");
		if(bany(&r->set))
			print(" s:%Q", r->set);
		if(bany(&r->use1))
			print(" u1:%Q", r->use1);
		if(bany(&r->use2))
			print(" u2:%Q", r->use2);
		if(bany(&r->refbehind))
			print(" rb:%Q ", r->refbehind);
		if(bany(&r->refahead))
			print(" ra:%Q ", r->refahead);
		if(bany(&r->calbehind))
			print("cb:%Q ", r->calbehind);
		if(bany(&r->calahead))
			print(" ca:%Q ", r->calahead);
		if(bany(&r->regdiff))
			print(" d:%Q ", r->regdiff);
		if(bany(&r->act))
			print(" a:%Q ", r->act);
	}
	print("\n");
}

void
dumpit(char *str, Reg *r0)
{
	Reg *r, *r1;

	print("\n%s\n", str);
	for(r = r0; r != R; r = r->link) {
		dumpone(r);
		r1 = r->p2;
		if(r1 != R) {
			print("	pred:");
			for(; r1 != R; r1 = r1->p2link)
				print(" %.4ud", r1->prog->loc);
			print("\n");
		}
//		r1 = r->s1;
//		if(r1 != R) {
//			print("	succ:");
//			for(; r1 != R; r1 = r1->s1)
//				print(" %.4ud", r1->prog->loc);
//			print("\n");
//		}
	}
}

static Sym*	symlist[10];

int
noreturn(Prog *p)
{
	Sym *s;
	int i;

	if(symlist[0] == S) {
		symlist[0] = pkglookup("panicindex", runtimepkg);
		symlist[1] = pkglookup("panicslice", runtimepkg);
		symlist[2] = pkglookup("throwinit", runtimepkg);
		symlist[3] = pkglookup("panic", runtimepkg);
	}

	s = p->to.sym;
	if(s == S)
		return 0;
	for(i=0; symlist[i]!=S; i++)
		if(s == symlist[i])
			return 1;
	return 0;
}
