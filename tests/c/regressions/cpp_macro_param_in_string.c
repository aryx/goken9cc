// claude: cpp_macro_param_in_string: found while unblocking benchs/
// compcert/mandelbrot.c -- `putc(byte_acc, stdout)` (via include/APE/
// stdio.h's `#define putc(c, stream) print("%c", (c))`) expanded to
// `print("%byte_acc", (byte_acc))` instead of `print("%c", (byte_acc))`:
// the macro's own parameter name `c` also appears as a character
// inside the string literal `"%c"` in the macro body, and the
// preprocessor wrongly substituted it there too.
//
// Root cause: the macro-body encoder (src/cmd/cc/macbody, compilers/
// cck/macbody, compilers/cc/mac.c -- all three, identically) scans the
// `#define`d body for occurrences of a parameter name to replace with
// an internal placeholder. The identifier-matching check
// (`if(isalpha(c) || c == '_')`) ran unconditionally, even while the
// scanner was inside a string or character literal (tracked by a
// local `ischr` variable, already correctly set/cleared for the rest
// of the loop) -- so a letter inside a string that happened to match a
// parameter name got treated as a reference to that parameter and
// substituted, even though C requires macro parameter substitution to
// never occur inside string or character literals.
//
// This checks both halves of the bug's shape at once, matching the
// real macro: a parameter name that also appears inside a string
// literal in the same macro body must leave that string untouched,
// while a genuine (non-string) use of the same parameter name
// elsewhere in the body must still substitute correctly. See
// docs/claude_notes/notes_shared_frontend_bugs.txt for the full
// writeup, including confirmation this was present identically in all
// three frontend forks this tree builds against.
//
// Confirmed real: exit(1)/(2) below, not exit(0), before any fix.
//
// Deliberately links against no libc (matches this directory's other
// bare-metal regressions, e.g. mulzero.c, arm64_vlong_if_branch.c):
// compares the expanded string by hand instead of calling strcmp(),
// so this needs only each arch's minimal _main/exit runtime stub.

#define MAC(c) mkpair("%c", (c))

typedef struct {
	char *fmt;
	int val;
} Pair;

Pair result;

extern void exit(int);

Pair *
mkpair(char *fmt, int val)
{
	result.fmt = fmt;
	result.val = val;
	return &result;
}

void
main(void)
{
	Pair *p;

	p = MAC(42);
	if (p->fmt[0] != '%' || p->fmt[1] != 'c' || p->fmt[2] != 0)
		exit(1);	// the bug: p->fmt would be "%42" instead
	if (p->val != 42)
		exit(2);	// substitution outside the string must still work
	exit(0);
}
