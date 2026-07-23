// Regression test for the wasm backend (compilers/ec), covering the
// first control-flow it supports: if/else (no loops yet -- see
// docs/notes_wasm.txt's "Open questions for ec"). No printf here (ec
// has no varargs/WASI runtime wired up yet, see the same file's gap
// list): runtests() below checks everything itself and returns 0 for
// all-pass or a bug-identifying nonzero code otherwise, verified by
// scripts/wasm-call-runner.js the way every other arch's hello*.exe
// is verified by `cmp` against an expected.txt.

// plain recursion + if/else, both branches returning (the case that
// needed compilers/ec/txt.c's AUNREACHABLE-after-terminal-if/else fix:
// without it, the trailing return every function gets from codgen()
// left an extra value on the wasm stack and failed validation).
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
	return 0;
}
