// claude: arm64_switch_vlong: found self-hosting compilers/7c via
// objtype=arm64 for the first time (mk/rc's own earlier self-hosting
// round only ever exercised the compiler as a TOOL, never compiling
// itself) -- compilers/7c/list.c has a real `switch(a->offset)` on a
// vlong-typed field, and 7c rejected it outright: "switch expression
// must be integer".
//
// Root cause: compilers/cck/pgen.c's OSWITCH case only accepts a
// controlling expression whose type is in the `typeswitch[]` table
// (cc.h/sub.c). compilers/7c/txt.c's own ginit() left
// `typeswitch = typechlv;` commented out with a stale "TODO: need
// also restore in cc.h" note -- typechlv (the char/short/long/vlong
// table, as opposed to typechl's char/short/long-only one) was
// already real and populated, nothing needed restoring. 6c's and 8c's
// own txt.c already had this line uncommented; 9front's 7c/txt.c has
// it too. See docs/claude_notes/notes_arch_arm64.txt.
//
// Deliberately uses a case value that does NOT fit in a 32-bit int
// (2^33) so a hypothetical future regression that silently truncated
// the switch expression to 32 bits before comparing (rather than
// outright rejecting it) would also be caught, not just the "does it
// compile at all" question. Deliberately does NOT also test a case
// set spread far enough apart to hit compilers/7c/swt.c's own,
// separate range-truncation bug -- see
// arm64_switch_vlong_wide_range.c for that one; keeping the two
// apart means a regression in either shows up as exactly one test
// failing, not both.
//
// Verify by hand:
//   7c -c arm64_switch_vlong.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_switch_vlong.exe -E _main \
//      arm64_switch_vlong.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_switch_vlong.exe; echo $?
// (want 0)

extern void exit(int);

long long
classify(long long v)
{
	switch(v){
	case 0:
		return 100;
	case 8589934592LL:		/* 2^33: doesn't fit a 32-bit int */
		return 200;
	default:
		return -1;
	}
}

void
main(void)
{
	if(classify(0) != 100)
		exit(1);
	if(classify(8589934592LL) != 200)
		exit(2);
	if(classify(1) != -1)
		exit(3);
	exit(0);
}
