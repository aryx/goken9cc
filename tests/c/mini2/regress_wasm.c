// Regression test for the wasm backend (compilers/ec), covering the
// control flow it supports so far: if/else and `while` (including
// break/continue/nesting) -- C `for` and `do while` are not
// implemented yet, see docs/notes_wasm.txt's "Open questions for ec"
// (their own entry jump lands inside the loop body from outside it,
// which needs loop rotation, not yet done -- compilers/ec/reg.c's
// validatescopes() catches the attempt with a clear diagnostic rather
// than emitting a module that fails wasm validation). No printf here
// (ec has no varargs/WASI runtime wired up yet, see the same file's
// gap list): runtests() below checks everything itself and returns 0
// for all-pass or a bug-identifying nonzero code otherwise, verified
// by scripts/wasm-call-runner.js the way every other arch's hello*.exe
// is verified by `cmp` against an expected.txt.

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
	return 0;
}
