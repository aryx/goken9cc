// A goto label on the direct body of an if: in C the label is
// transparent, so error() must run only when c == 0. The principia
// cc parsed 'LNAME :' as a ulstmnt alternative instead of a label
// alternative, so the bare label alone became the if body and the
// call was hoisted out of the if: BOTH paths called error()
// (kencc: JNE ,5(PC) skipping the body; principia: JNE ,2(PC) to
// the label, then falling into the call anyway). Harmless when the
// label sits inside a block (stmnt: labels ulstmnt covers it),
// broken when it labels a direct if/while/for body. Found on the
// principia corpus (kernel/init/builtin.c, libregexp/regexp.c,
// libimg/readjpg.c).
//
// Cause: an LP chunk reorder in principia commit 76e5148a
// (2016-12-19, "lots of LP split and aspectize") spliced the
// 'label rule' chunk so the LNAME alternative sat before the
// 'label:' production header, attaching it to the still-open
// ulstmnt production. Original plan9 (and kencc) have LNAME ':'
// as the third alternative of label.

void error(char*);

int
f(int c)
{
	if(c == 0)
err:
		error("boom");
	return c;
}
