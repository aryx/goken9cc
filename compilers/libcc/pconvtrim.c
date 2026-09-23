/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * Shared by both frontend lineages (compilers/libcc for 5c/8c,
 * compilers/cck for 6c/7c/vc/ic -- cck's mkfile compiles this file
 * from here), so it deliberately includes only u.h/libc.h and not
 * either lineage's own cc.h.
 */
#include <u.h>
#include <libc.h>

/*
 * claude: every backend's Pconv prints a fixed number of
 * comma-separated operand slots, even when D_NONE (Dconv) renders one
 * of them as an empty string -- e.g. "RET\t," or "BL\t,print+0(SB)".
 * That is not valid assembler input (feeding such -S output back into
 * the matching Xa rejects it with a syntax error); it's a debug-print
 * artifact. Strip a dangling leading or trailing comma from the
 * operand field (the text after the second tab) so -S output is
 * reassemblable. Found while using goken's -S output as input to
 * xix's oNa (see xix's docs/claude_notes/done/plan_hello_libc_linking.md).
 */
void
pconvtrim(char *str)
{
	char *t1, *t2, *end;

	t1 = strchr(str, '\t');
	if(t1 == nil || (t2 = strchr(t1+1, '\t')) == nil)
		return;
	if(t2[1] == ',')
		memmove(t2+1, t2+2, strlen(t2+2)+1);
	end = str+strlen(str);
	while(end > str && (end[-1] == '\n' || end[-1] == '\r'))
		end--;
	if(end > str && end[-1] == ',')
		memmove(end-1, end, strlen(end)+1);
}
