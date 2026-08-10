// Regression test for the wasm backend (compilers/ec), covering the
// control flow it supports so far: if/else, `while` (including break/
// continue/nesting), `for`/`do while` (including break/continue/
// arbitrarily deep nesting -- the latter two need loop rotation,
// compilers/ec/reg.c's rotateloops(), see docs/notes_wasm.txt, since
// their own entry jump lands inside the loop body from outside it,
// which plain block/loop nesting can't express directly), and switch/
// case -- standalone (classify_switch()) and, since reg.c's
// hoistswitches(), inside a loop that also has its own backward branch
// like a `continue` (loopswitch()); see notes_wasm.txt for how that
// used to be a real, open relooper limitation. Also: pointer
// dereference (OIND), increment/decrement and compound assignment (on
// a plain ONAME lvalue), address-of a global, and the variadic-call
// ABI (a variadic function's own named parameter, plus its "..."
// actuals). No printf here even so (print_nofloat_no64.c's own
// vprintf() -- exactly loopswitch()'s shape -- still needs two more
// separate, still-open pieces: initialized global/static data, and
// el linking two input files that each have their own string-literal
// data, see notes_wasm.txt): runtests() below checks everything itself
// and returns 0 for all-pass or a bug-identifying nonzero code
// otherwise, verified by scripts/wasm-call-runner.js the way every
// other arch's hello*.exe is verified by `cmp` against an expected.txt.

// plain recursion + if/else, both branches returning -- exercises
// compilers/ec/reg.c's dead-code elision: codgen() always appends a
// trailing return whether or not the function already returned on
// every path, and here it has (both if/else branches return), so that
// trailing return must never actually reach the wasm output.
int
fact(int n)
{
	if (n == 0) {
		return 1;
	} else {
		return n * fact(n - 1);
	}
}

// if with no else at all
int
abs1(int n)
{
	if (n < 0) {
		n = 0 - n;
	}
	return n;
}

// if/else where *neither* branch returns (falls through to a shared
// return afterward) -- checks that AUNREACHABLE is correctly *not*
// inserted when at least one branch isn't terminal.
int
pick(int a, int b, int useb)
{
	int m;

	m = a;
	if (useb) {
		m = b;
	} else {
		m = a;
		m = m;
	}
	return m;
}

// nested if/else (an "else if" chain is just this, parsed by the
// frontend) -- checks the if-stack's own nesting discipline.
int
classify(int n)
{
	if (n == 0) {
		return 0;
	} else if (n > 0) {
		return 1;
	} else {
		return -1;
	}
}

// plain while -- the entry jump and the back-edge (via `continue`'s
// own pc snapshot) land on the *same* position (the test), unlike
// for/do-while, so this doesn't hit the "branch into a loop's
// interior" limitation those do.
int
sumto(int n)
{
	int s, i;

	s = 0;
	i = 1;
	while (i <= n) {
		s = s + i;
		i = i + 1;
	}
	return s;
}

// while + break: checks that breakpc's own trampoline threads to a
// position *outside* (after) the loop correctly.
int
firstge(int n, int limit)
{
	int i;

	i = 0;
	while (1) {
		if (i >= limit) {
			break;
		}
		i = i + 1;
		if (i > n) {
			break;
		}
	}
	return i;
}

// while + continue: checks continpc's own trampoline threads to the
// test (not the top of the loop body) correctly.
int
sumskip3(int n)
{
	int i, s;

	i = 0;
	s = 0;
	while (i < n) {
		i = i + 1;
		if (i == 3) {
			continue;
		}
		s = s + i;
	}
	return s;
}

// nested while: checks that the inner loop's own scope nests inside
// the outer one instead of the two crossing.
int
nestedcount(int n)
{
	int i, j, s;

	s = 0;
	i = 0;
	while (i < n) {
		j = 0;
		while (j < n) {
			s = s + 1;
			j = j + 1;
		}
		i = i + 1;
	}
	return s;
}

// if/else inside a while body: checks that the if's own wrapping
// block nests inside the loop's, not the reverse (extendscopes()'s
// widening only ever pulls a block *out*, never a loop).
int
sumspecial(int n)
{
	int i, s;

	s = 0;
	i = 0;
	while (i < n) {
		if (i == 2) {
			s = s + 10;
		} else {
			s = s + 1;
		}
		i = i + 1;
	}
	return s;
}

// plain `for`: exercises loop rotation's simplest case (a single,
// non-nested loop whose own automatic back-edge is a real, live
// instruction sitting right where rotateonce() expects it).
int
forsum(int n)
{
	int i, s;

	s = 0;
	for (i = 0; i < n; i = i + 1) {
		s = s + i;
	}
	return s;
}

// `do while`: rotation's PREFIX/REST split is the other way round
// from `for` (PREFIX is the test, REST is the body) -- see
// rotateonce()'s comment.
int
dowhilesum(int n)
{
	int i, s;

	s = 0;
	i = 0;
	do {
		s = s + i;
		i = i + 1;
	} while (i < n);
	return s;
}

int
forbreak(int n)
{
	int i, s;

	s = 0;
	for (i = 0; i < n; i = i + 1) {
		if (i == 3) {
			break;
		}
		s = s + 1;
	}
	return s;
}

int
forcontinue(int n)
{
	int i, s;

	s = 0;
	for (i = 0; i < n; i = i + 1) {
		if (i == 2) {
			continue;
		}
		s = s + 1;
	}
	return s;
}

// a `for` nested directly in another `for`'s body: the outer loop's
// own automatic back-edge gets jump-threaded away as dead code (the
// inner loop's own conditional test-exit already reaches the outer
// increment directly), so there is no live instruction left to
// hardcode-retarget the way rotateonce() does for the simple case --
// rotateonce_insert() synthesizes the missing instruction instead. See
// findrotation()'s Dpos comment and rotateonce_insert()'s own comment
// for the full story (this was the trickiest bug in the whole
// rotation pass).
int
nestedfor(int n)
{
	int i, j, s;

	s = 0;
	for (i = 0; i < n; i = i + 1) {
		for (j = 0; j < n; j = j + 1) {
			s = s + 1;
		}
	}
	return s;
}

// three levels deep: findrotation()'s "always rotate the smallest-
// span (innermost) violation first" rule has to hold up recursively,
// not just for one level of nesting.
int
triplenestedfor(int n)
{
	int i, j, k, s;

	s = 0;
	for (i = 0; i < n; i = i + 1) {
		for (j = 0; j < n; j = j + 1) {
			for (k = 0; k < n; k = k + 1) {
				s = s + 1;
			}
		}
	}
	return s;
}

// a `do while` nested directly in another `do while`'s body -- same
// underlying issue as nestedfor() above, just with PREFIX/REST
// swapped per do-while's own shape.
int
nesteddowhile(int n)
{
	int i, j, s;

	s = 0;
	i = 0;
	do {
		j = 0;
		do {
			s = s + 1;
			j = j + 1;
		} while (j < n);
		i = i + 1;
	} while (i < n);
	return s;
}

// pointer dereference (OIND), read and write. Goes through a global
// rather than a local's address -- address-of-a-local is still
// unimplemented (see docs/notes_wasm.txt's "Open questions for ec")
// -- purely to keep this test decoupled from that separate, still-open
// question; OADDR-of-a-global is already implemented and tested.
int gscratch;

int
deref(int *p, int v)
{
	*p = v;
	return *p + 1;
}

// increment/decrement on a local int -- both the POST (result = old
// value) and PRE (result = new value) forms, each checked against
// both the returned expression value *and* the variable's final state
// (`r*100+i` packs both into one checked int).
int
postinc_local(void)
{
	int i, r;

	i = 5;
	r = i++;
	return r*100 + i;
}

int
postdec_local(void)
{
	int i, r;

	i = 5;
	r = i--;
	return r*100 + i;
}

int
preinc_local(void)
{
	int i, r;

	i = 5;
	r = ++i;
	return r*100 + i;
}

int
predec_local(void)
{
	int i, r;

	i = 5;
	r = --i;
	return r*100 + i;
}

// pointer increment scales by the pointee's width (unlike the plain
// int case above) -- exactly the `p++` shape print_nofloat_no64.c's
// own vprintf() uses. Checked via a pointer-to-int cast rather than
// pointer subtraction (not itself exercised here) or array indexing
// (still unimplemented, see docs/notes_wasm.txt).
int
ptrincword(void)
{
	int *p;
	int before, after;

	p = &gscratch;
	before = (int)p;
	p++;
	after = (int)p;
	return after - before;
}

int
ptrincbyte(void)
{
	char *p;
	int before, after;

	p = (char*)&gscratch;
	before = (int)p;
	p++;
	after = (int)p;
	return after - before;
}

// compound assignment: result is the *new* value, on both a local
// (ALOCALTEE path) and a global (gaddr()+re-fetch path) lvalue.
// asdiv_local's `v` is unsigned specifically to exercise cck/com.c's
// own OASDIV->OASLDIV rewrite (print_nofloat_no64.c's printhex() does
// the same `v /= 16` on a uint32).
int
asadd_local(void)
{
	int x;

	x = 10;
	x += 5;
	return x;
}

int
asdiv_local(void)
{
	unsigned int v;

	v = 100;
	v /= 3;
	return v;
}

int
asadd_global(void)
{
	gscratch = 10;
	gscratch += 5;
	return gscratch;
}

// the variadic-call ABI itself -- a dedicated function (not printf,
// to keep this self-checking via return code rather than stdout) that
// reads `n` more int arguments the exact same way
// print_nofloat_no64.c's own vprintf() reads its varargs: take the
// address of the last *named* parameter and walk forward from there.
// Exercises both sides of the ABI at once: cgen.c's OFUNC/
// callvariadic() (caller: marshals every actual argument, named and
// "...", into a shadow-stack buffer and passes its base address) and
// txt.c's isvarargparam()/lload() (callee: reads `n` back out of that
// same buffer instead of expecting a real wasm local for it).
int
sumvariadic(int n, ...)
{
	int *arg;
	int i, s;

	arg = (int*)(&n + 1);
	s = 0;
	for (i = 0; i < n; i = i + 1) {
		s = s + *arg;
		arg = arg + 1;
	}
	return s;
}

// switch/case (swit1() in txt.c) -- multiple case labels sharing one
// body, a default, and a plain (non-loop) context. See loopswitch()
// below for the same construct *inside* a loop that also has its own
// backward branch -- a real, deeper limitation this used to hit
// unconditionally (see notes_wasm.txt), now fixed by reg.c's
// hoistswitches().
int
classify_switch(int n)
{
	int r;

	switch (n) {
	case 0:
	case 1:
		r = 10;
		break;
	case 2:
		r = 20;
		break;
	default:
		r = -1;
	}
	return r;
}

// switch(es) *inside* a `for` loop that also has its own backward
// branch (`continue`) -- the exact shape classify_switch()'s own
// comment used to flag as a real, open relooper limitation (a switch's
// case-dispatch branches are backward too, per doswit()'s body-then-
// dispatch ordering, and used to get misclassified as overlapping with
// the loop's own back-edge). Fixed by reg.c's hoistswitches(): each
// switch's whole compare-and-dispatch chain gets physically relocated
// before its own body (swt.c's swit1() records the span via
// recordswitch()), turning every case-label branch into an ordinary
// *forward* one before buildscopes() ever sees it. This is exactly
// print_nofloat_no64.c's vprintf() shape (a `for` with `continue`,
// containing two back-to-back switches) -- see notes_wasm.txt.
int
loopswitch(int n)
{
	int i, sum;

	sum = 0;
	for (i = 0; i < n; i = i + 1) {
		if (i == 1)
			continue;
		switch (i) {
		case 0:
		case 2:
			sum = sum + 1;
			break;
		}
		switch (i) {
		case 0:
			sum = sum + 100;
			break;
		case 3:
			sum = sum + 200;
			break;
		case 4:
			sum = sum + 300;
			break;
		}
	}
	return sum;
}

int
runtests(void)
{
	if (fact(0) != 1)
		return 1;
	if (fact(5) != 120)
		return 2;
	if (abs1(-7) != 7)
		return 3;
	if (abs1(7) != 7)
		return 4;
	if (pick(3, 9, 0) != 3)
		return 5;
	if (pick(3, 9, 1) != 9)
		return 6;
	if (classify(0) != 0)
		return 7;
	if (classify(9) != 1)
		return 8;
	if (classify(-5) != -1)
		return 9;
	if (sumto(5) != 15)
		return 10;
	if (sumto(0) != 0)
		return 11;
	if (firstge(10, 4) != 4)
		return 12;
	if (firstge(2, 10) != 3)
		return 13;
	if (sumskip3(5) != 12)
		return 14;
	if (nestedcount(3) != 9)
		return 15;
	if (sumspecial(5) != 14)
		return 16;
	if (forsum(5) != 10)
		return 17;
	if (dowhilesum(5) != 10)
		return 18;
	if (forbreak(5) != 3)
		return 19;
	if (forcontinue(5) != 4)
		return 20;
	if (nestedfor(3) != 9)
		return 21;
	if (nestedfor(4) != 16)
		return 22;
	if (triplenestedfor(3) != 27)
		return 23;
	if (nesteddowhile(3) != 9)
		return 24;
	if (deref(&gscratch, 41) != 42)
		return 25;
	if (postinc_local() != 506)
		return 26;
	if (postdec_local() != 504)
		return 27;
	if (preinc_local() != 606)
		return 28;
	if (predec_local() != 404)
		return 29;
	if (ptrincword() != 4)
		return 30;
	if (ptrincbyte() != 1)
		return 31;
	if (asadd_local() != 15)
		return 32;
	if (asdiv_local() != 33)
		return 33;
	if (asadd_global() != 15)
		return 34;
	if (sumvariadic(3, 10, 20, 30) != 60)
		return 35;
	if (sumvariadic(0) != 0)
		return 36;
	if (classify_switch(0) != 10)
		return 37;
	if (classify_switch(1) != 10)
		return 38;
	if (classify_switch(2) != 20)
		return 39;
	if (classify_switch(9) != -1)
		return 40;
	if (loopswitch(5) != 602)
		return 41;
	return 0;
}
