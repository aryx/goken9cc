#include "gc.h"

void
swit1(C1 *q, int nc, int32 def, Node *n)
{
	Node tn;
	
	regalloc(&tn, &regnode, Z);
	swit2(q, nc, def, n, &tn);
	regfree(&tn);
}

void
swit2(C1 *q, int nc, int32 def, Node *n, Node *tn)
{
	C1 *r;
	int i;
	vlong v;
	vlong range;
	Prog *sp;

	/* claude: was `int i = (q+nc-1)->val - (q+0)->val;`, reusing the
	 * SAME `i` this function's other loops use as a plain 0..nc-1
	 * counter -- a new, distinctly-named `vlong range` instead, both
	 * to avoid that overload and because the old `int` truncated any
	 * genuinely 64-bit case-value spread. Found self-hosting
	 * compilers/7c: switch(vlong) with case values 2^33 and 2^33+1 (a
	 * spread of 8589934593, nowhere near "dense") truncated to `1`,
	 * wrongly taking the `direct` jump-table path below meant only for
	 * a tightly clustered case set. `v` (used only in that `direct`
	 * path, no reuse-elsewhere concern) widened to vlong for the same
	 * reason: it used to start at `q->val` truncated to int32, which
	 * for a base case value like 2^33 (0 mod 2^32) starts the
	 * increment loop already wrong, and increments one at a time
	 * toward a `q->val` comparison that -- constrained to int32's own
	 * range -- it can now never reach: an infinite loop, not just a
	 * wrong table, confirmed by reproducing it directly (7c hung
	 * indefinitely on exactly this switch shape). Confirmed against
	 * 9front's own compilers/7c/swt.c, which has the identical
	 * `int i` truncation (its own `v` is a real `long`, 64-bit on
	 * Plan9's own arm64, so it doesn't share the second half of this
	 * bug) -- not confirmed hit upstream, not blindly matched here.
	 * See tests/c/regressions/arm64_switch_vlong_wide_range.c. */
	if(nc >= 3) {
		range = (q+nc-1)->val - (q+0)->val;
		if(range > 0 && range < nc*2)
			goto direct;
	}
	/* claude: nodgconst (not nodconst, int32-only) in all three spots
	 * below too -- same bug and same fix as the `direct:` label further
	 * down (see this function's own header comment): q->val/r->val are
	 * real vlong case values, and nodconst() silently truncates
	 * whatever it's given to int32. This half didn't hang (unlike the
	 * `direct` path's runaway v++ loop) -- it just silently compared
	 * against the wrong, truncated constant, so switch(vlong) with
	 * case values that don't fit 32 bits still returned wrong answers
	 * even after the `direct`-path infinite loop was fixed (caught by
	 * tests/c/regressions/arm64_switch_vlong_wide_range.c, run range
	 * >= nc*2 so this file's own bug stays isolated from that one). */
	if(nc < 5) {
		for(i=0; i<nc; i++) {
			if(debug['K'])
				print("case = %.8llux\n", q->val);
			gopcode(OEQ, nodgconst(q->val, n->type), n, Z);
			patch(p, q->label);
			q++;
		}
		gbranch(OGOTO);
		patch(p, def);
		return;
	}

	i = nc / 2;
	r = q+i;
	if(debug['K'])
		print("case > %.8llux\n", r->val);
	gopcode(OGT, nodgconst(r->val, n->type), n, Z);
	sp = p;
	gopcode(OEQ, nodgconst(r->val, n->type), n, Z);	/* just gen the B.EQ */
	patch(p, r->label);
	swit2(q, i, def, n, tn);

	if(debug['K'])
		print("case < %.8llux\n", r->val);
	patch(sp, pc);
	swit2(r+1, nc-i-1, def, n, tn);
	return;

direct:
	/* claude: nodgconst (not nodconst, int32-only) -- v is a real
	 * vlong now (see this function's own header comment); nodgconst
	 * itself dispatches to a genuine 64-bit constant node when n's own
	 * type is vlong-shaped, falling back to nodconst's plain 32-bit
	 * path otherwise (exactly the n->type check doswit()'s own isv
	 * flag already established for this whole switch), so this stays
	 * correct for a plain 32-bit switch too, not just the vlong case
	 * that exposed the bug. */
	v = q->val;
	if(v != 0)
		gopcode(OSUB, nodgconst(v, n->type), Z, n);
	gopcode(OHI, nodgconst((q+nc-1)->val - v, n->type), n, Z);
	patch(p, def);
	gopcode(OCASE, n, Z, tn);
	for(i=0; i<nc; i++) {
		if(debug['K'])
			print("case = %.8llux\n", q->val);
		while(q->val != v) {
			nextpc();
			p->as = ABCASE;
			patch(p, def);
			v++;
		}
		nextpc();
		p->as = ABCASE;
		patch(p, q->label);
		q++;
		v++;
	}
	gbranch(OGOTO);		/* so that regopt() won't be confused */
	patch(p, def);
}

void
bitload(Node *b, Node *n1, Node *n2, Node *n3, Node *nn)
{
	int sh;
	int32 v;
	Node *l;

	/*
	 * n1 gets adjusted/masked value
	 * n2 gets address of cell
	 * n3 gets contents of cell
	 */
	l = b->left;
	if(n2 != Z) {
		regalloc(n1, l, nn);
		reglcgen(n2, l, Z);
		regalloc(n3, l, Z);
		gopcode(OAS, n2, Z, n3);
		gopcode(OAS, n3, Z, n1);
	} else {
		regalloc(n1, l, nn);
		cgen(l, n1);
	}
	if(b->type->shift == 0 && typeu[b->type->etype]) {
		v = ~0 + (1L << b->type->nbits);
		gopcode(OAND, nodconst(v), Z, n1);
	} else {
		sh = 32 - b->type->shift - b->type->nbits;
		if(sh > 0)
			gopcode(OASHL, nodconst(sh), Z, n1);
		sh += b->type->shift;
		if(sh > 0)
			if(typeu[b->type->etype])
				gopcode(OLSHR, nodconst(sh), Z, n1);
			else
				gopcode(OASHR, nodconst(sh), Z, n1);
	}
}

void
bitstore(Node *b, Node *n1, Node *n2, Node *n3, Node *nn)
{
	int32 v;
	Node nod, *l;
	int sh;

	/*
	 * n1 has adjusted/masked value
	 * n2 has address of cell
	 * n3 has contents of cell
	 */
	l = b->left;
	regalloc(&nod, l, Z);
	v = ~0 + (1L << b->type->nbits);
	gopcode(OAND, nodconst(v), Z, n1);
	gopcode(OAS, n1, Z, &nod);
	if(nn != Z)
		gopcode(OAS, n1, Z, nn);
	sh = b->type->shift;
	if(sh > 0)
		gopcode(OASHL, nodconst(sh), Z, &nod);
	v <<= sh;
	gopcode(OAND, nodconst(~v), Z, n3);
	gopcode(OOR, n3, Z, &nod);
	gopcode(OAS, &nod, Z, n2);

	regfree(&nod);
	regfree(n1);
	regfree(n2);
	regfree(n3);
}

long
outstring(char *s, long n)
{
	int32 r;

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

int
mulcon(Node *n, Node *nn)
{
	Node *l, *r, nod1, nod2;
	Multab *m;
	int32 v, vs;
	int o;
	char code[sizeof(m->code)+2], *p;

	if(typefd[n->type->etype])
		return 0;
	l = n->left;
	r = n->right;
	if(l->op == OCONST) {
		l = r;
		r = n->left;
	}
	if(r->op != OCONST)
		return 0;
	v = convvtox(r->vconst, n->type->etype);
	if(v != r->vconst) {
		if(debug['M'])
			print("%L multiply conv: %lld\n", n->lineno, r->vconst);
		return 0;
	}
	m = mulcon0(v);
	if(!m) {
		if(debug['M'])
			print("%L multiply table: %lld\n", n->lineno, r->vconst);
		return 0;
	}
	if(debug['M'] && debug['v'])
		print("%L multiply: %ld\n", n->lineno, v);

	memmove(code, m->code, sizeof(m->code));
	code[sizeof(m->code)] = 0;

	p = code;
	if(p[1] == 'i')
		p += 2;
	regalloc(&nod1, n, nn);
	cgen(l, &nod1);
	vs = v;
	regalloc(&nod2, n, Z);

loop:
	switch(*p) {
	case 0:
		regfree(&nod2);
		if(vs < 0) {
			gopcode(OAS, &nod1, Z, &nod1);
			gopcode(ONEG, &nod1, Z, nn);
		} else 
			gopcode(OAS, &nod1, Z, nn);
		regfree(&nod1);
		return 1;
	case '+':
		o = OADD;
		goto addsub;
	case '-':
		o = OSUB;
	addsub:	/* number is r,n,l */
		v = p[1] - '0';
		r = &nod1;
		if(v&4)
			r = &nod2;
		n = &nod1;
		if(v&2)
			n = &nod2;
		l = &nod1;
		if(v&1)
			l = &nod2;
		gopcode(o, l, n, r);
		break;
	default: /* op is shiftcount, number is r,l */
		v = p[1] - '0';
		r = &nod1;
		if(v&2)
			r = &nod2;
		l = &nod1;
		if(v&1)
			l = &nod2;
		v = *p - 'a';
		if(v < 0 || v >= 32) {
			diag(n, "mulcon unknown op: %c%c", p[0], p[1]);
			break;
		}
		gopcode(OASHL, nodconst(v), l, r);
		break;
	}
	p += 2;
	goto loop;
}

void
gextern(Sym *s, Node *a, long o, long w)
{

	if(a->op == OCONST && typev[a->type->etype]) {
		if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
			gpseudo(ADATA, s, nod32const(a->vconst>>32));
		else
			gpseudo(ADATA, s, nod32const(a->vconst));
		p->from.offset += o;
		p->reg = 4;
		if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
			gpseudo(ADATA, s, nod32const(a->vconst));
		else
			gpseudo(ADATA, s, nod32const(a->vconst>>32));
		p->from.offset += o + 4;
		p->reg = 4;
		return;
	}
	gpseudo(ADATA, s, a);
	p->from.offset += o;
	p->reg = w;
	if(p->to.type == D_OREG)
		p->to.type = D_CONST;
}

void	zname(Biobuf*, Sym*, int);
char*	zaddr(char*, Adr*, int);
void	zwrite(Biobuf*, Prog*, int, int);
void	outhist(Biobuf*);

void
zwrite(Biobuf *b, Prog *p, int sf, int st)
{
	char bf[100], *bp;
	int32 l;

	bf[0] = p->as;
	bf[1] = p->as>>8;
	bf[2] = p->reg;
	l = p->lineno;
	bf[3] = l;
	bf[4] = l>>8;
	bf[5] = l>>16;
	bf[6] = l>>24;
	bp = zaddr(bf+7, &p->from, sf);
	bp = zaddr(bp, &p->to, st);
	Bwrite(b, bf, bp-bf);
}

void
outcode(void)
{
	struct { Sym *sym; short type; } h[NSYM];
	Prog *p;
	Sym *s;
	int sf, st, t, sym;

	if(debug['S']) {
		for(p = firstp; p != P; p = p->link)
			if(p->as != ADATA && p->as != AGLOBL)
				pc--;
		for(p = firstp; p != P; p = p->link) {
			print("%P\n", p);
			if(p->as != ADATA && p->as != AGLOBL)
				pc++;
		}
	}
	outhist(&outbuf);
	for(sym=0; sym<NSYM; sym++) {
		h[sym].sym = S;
		h[sym].type = 0;
	}
	sym = 1;
	for(p = firstp; p != P; p = p->link) {
	jackpot:
		sf = 0;
		s = p->from.sym;
		while(s != S) {
			sf = s->sym;
			if(sf < 0 || sf >= NSYM)
				sf = 0;
			t = p->from.name;
			if(h[sf].type == t)
			if(h[sf].sym == s)
				break;
			s->sym = sym;
			zname(&outbuf, s, t);
			h[sym].sym = s;
			h[sym].type = t;
			sf = sym;
			sym++;
			if(sym >= NSYM)
				sym = 1;
			break;
		}
		st = 0;
		s = p->to.sym;
		while(s != S) {
			st = s->sym;
			if(st < 0 || st >= NSYM)
				st = 0;
			t = p->to.name;
			if(h[st].type == t)
			if(h[st].sym == s)
				break;
			s->sym = sym;
			zname(&outbuf, s, t);
			h[sym].sym = s;
			h[sym].type = t;
			st = sym;
			sym++;
			if(sym >= NSYM)
				sym = 1;
			if(st == sf)
				goto jackpot;
			break;
		}
		zwrite(&outbuf, p, sf, st);
	}
	firstp = P;
	lastp = P;
}

void
outhist(Biobuf *b)
{
	Hist *h;
	char *p, *q, *op, c;
	Prog pg;
	int n;

	pg = zprog;
	pg.as = AHISTORY;
	c = pathchar();
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		/* on windows skip drive specifier in pathname */
		if(systemtype(Windows) && p && p[1] == ':'){
			p += 2;
			c = *p;
		}
		if(p && p[0] != c && h->offset == 0 && pathname){
			/* on windows skip drive specifier in pathname */
			if(systemtype(Windows) && pathname[1] == ':') {
				op = p;
				p = pathname+2;
				c = *p;
			} else if(pathname[0] == c){
				op = p;
				p = pathname;
			}
		}
		while(p) {
			q = utfrune(p, c);
			if(q) {
				n = q-p;
				if(n == 0){
					n = 1;	/* leading "/" */
					*p = '/';	/* don't emit "\" on windows */
				}
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(b, ANAME);
				Bputc(b, ANAME>>8);
				Bputc(b, D_FILE);
				Bputc(b, 1);
				Bputc(b, '<');
				Bwrite(b, p, n);
				Bputc(b, 0);
			}
			p = q;
			if(p == 0 && op) {
				p = op;
				op = 0;
			}
		}
		pg.lineno = h->line;
		pg.to.type = zprog.to.type;
		pg.to.offset = h->offset;
		if(h->offset)
			pg.to.type = D_CONST;

		zwrite(b, &pg, 0, 0);
	}
}

void
zname(Biobuf *b, Sym *s, int t)
{
	char *n, bf[8];
	uint32 sig;

	n = s->name;
	if(debug['T'] && t == D_EXTERN && s->sig != SIGDONE && s->type != types[TENUM] && s != symrathole){
		sig = sign(s);
		bf[0] = ASIGNAME & 0xFF;
		bf[1] = ASIGNAME>>8;
		bf[2] = sig;
		bf[3] = sig>>8;
		bf[4] = sig>>16;
		bf[5] = sig>>24;
		bf[6] = t;
		bf[7] = s->sym;
		Bwrite(b, bf, 8);
		s->sig = SIGDONE;
	}
	else{
		bf[0] = ANAME & 0xFF;
		bf[1] = ANAME>>8;
		bf[2] = t;	/* type */
		bf[3] = s->sym;	/* sym */
		Bwrite(b, bf, 4);
	}
	Bwrite(b, n, strlen(n)+1);
}

char*
zaddr(char *bp, Adr *a, int s)
{
	int32 l;
	Ieee e;

	if(a->type == D_CONST){
		l = a->offset;
		if((vlong)l != a->offset)
			a->type = D_DCONST;
	}
	bp[0] = a->type;
	bp[1] = a->reg;
	bp[2] = s;
	bp[3] = a->name;
	bp += 4;
	switch(a->type) {
	default:
		diag(Z, "unknown type %d in zaddr", a->type);

	case D_NONE:
	case D_REG:
	case D_SP:
	case D_FREG:
	case D_VREG:
		break;

	case D_PAIR:
		*bp++ = a->offset;
		break;

	case D_OREG:
	case D_XPRE:
	case D_XPOST:
	case D_CONST:
	case D_BRANCH:
	case D_SHIFT:
	case D_EXTREG:
	case D_SPR:
		l = a->offset;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		break;

	case D_DCONST:
		l = a->offset;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		l = a->offset>>32;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		break;

	case D_SCONST:
		memmove(bp, a->sval, NSNAME);
		bp += NSNAME;
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		l = e.l;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		l = e.h;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		break;
	}
	return bp;
}

int32
align(int32 i, Type *t, int op)
{
	int32 o;
	Type *v;
	int w;

	o = i;
	w = 1;
	switch(op) {
	default:
		diag(Z, "unknown align opcode %d", op);
		break;

	case Asu2:	/* padding at end of a struct */
		w = SZ_VLONG;
		if(packflg)
			w = packflg;
		break;

	case Ael1:	/* initial align of struct element */
		for(v=t; v->etype==TARRAY; v=v->link)
			;
		w = ewidth[v->etype];
		if(w <= 0 || w >= SZ_VLONG)
			w = SZ_VLONG;
		if(packflg)
			w = packflg;
		break;

	case Ael2:	/* width of a struct element */
		o += t->width;
		break;

	case Aarg0:	/* initial passbyptr argument in arg list */
		if(typesu[t->etype]) {
			o = align(o, types[TIND], Aarg1);
			o = align(o, types[TIND], Aarg2);
		}
		break;

	case Aarg1:	/* initial align of parameter */
		w = ewidth[t->etype];
		if(w <= 0 || w >= SZ_VLONG) {
			w = SZ_VLONG;
			break;
		}
		w = 1;		/* little endian no adjustment */
		break;

	case Aarg2:	/* width of a parameter */
		o += t->width;
		w = SZ_VLONG;
		break;

	case Aaut3:	/* total align of automatic */
		o = align(o, t, Ael2);
		o = align(o, t, Ael1);
		w = SZ_LONG;	/* because of a pun in cc/dcl.c:contig() */
		break;
	}
	o = round(o, w);
	if(debug['A'])
		print("align %s %ld %T = %ld\n", bnames[op], i, t, o);
	return o;
}

int32
maxround(int32 max, int32 v)
{
	v = round(v, SZ_VLONG);
	if(v > max)
		return v;
	return max;
}
