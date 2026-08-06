// claude: arm64_fmovd_large_offset: found self-hosting compilers/7c via
// objtype=arm64 for the first time -- 7c's own float-constant table
// (fconstnode) and mpatof.c's pows10<> table land past this libc's
// 64MB minimal_malloc.c heap[] in the data segment, needing an
// SB-relative FMOVD access whose offset exceeds a single ADD-
// immediate's ~16MB reach. Three real, separate bugs, all found and
// fixed getting this one scenario to work:
//
// 1. linkers/7l/optab.c had zero C_LEXT/C_LAUTO/C_LOREG rows for
//    AFMOVD (double), unlike its AFMOVS (float) sibling right above --
//    "illegal combination FMOVD FREG NONE LEXT". Confirmed against
//    9front's own optab.c, which has these same six rows.
// 2. linkers/7l/asmout.c case 30/31's own bailout (`if(v < 0 ||
//    (v>>s) >= (1<<24)) goto Hugestxr/Hugeldxr;`, the fix for
//    arm64_large_bss_sb_offset.c above) used the WRONG threshold for
//    any scale s>0: hi (the ADD-immediate half of the address
//    computation) needs hi < 1<<24 regardless of s, not v>>s < 1<<24
//    -- the `>>s` let offsets up to 8x too large through for FMOVD's
//    s=3, which then failed *inside* oaddi() ("offset out of range")
//    instead of routing to the huge-offset fallback. 9front has the
//    identical `>>s` bug (unexercised there, not confirmed fixed
//    upstream).
// 3. linkers/7l/asmout.c's olsxrr()/opldrpp() (the huge-offset
//    fallback's own encoder, case 47/48, "never reached until now")
//    had no AFMOVS/AFMOVD cases at all -- "bad opldr FMOVD". Ported
//    from 9front's own opldrpp(), which already has both.
//
// See docs/claude_notes/notes_arch_arm64.txt for the full writeup.
//
// Deliberately declares more than 64MB of file-scope char arrays
// ahead of the double constants under test, forcing the linker to
// place them past the large-offset threshold regardless of dodata()'s
// own hash-table-iteration-order (the same non-determinism
// arm64_large_bss_sb_offset.c's own header comment already warns
// about) -- padding is not fussy about exact size, just "comfortably
// past 16MB, the smallest threshold any of the three bugs above cares
// about".
//
// Verify by hand:
//   7c -c arm64_fmovd_large_offset.c
//   7a -c arm64_regressions_start.s
//   7l -H7 -o arm64_fmovd_large_offset.exe -E _main \
//      arm64_fmovd_large_offset.7 arm64_regressions_start.7
//   ../../../scripts/qemu-runner arm64 ./arm64_fmovd_large_offset.exe; echo $?
// (want 0)

extern void exit(int);

/* claude: no libc linked here (see arm64_regressions_start.s's own
 * header comment, shared by every .check in this mkfile) -- a plain
 * inline absolute-difference check instead of calling fabs(). */
static int
neq(double a, double b, double tol)
{
	double d;

	d = a - b;
	if(d < 0)
		d = -d;
	return d > tol;
}

enum { PAD = 20*1024*1024 };	/* > 16MB: past a single ADD-immediate's reach */

char padding[PAD];

double dconst = 3.14159265358979;
double dconst2 = 2.71828182845905;

void
main(void)
{
	double a, b, c;

	padding[0] = 1;
	padding[PAD-1] = 2;

	a = dconst;		/* FMOVD load, large SB-offset */
	b = dconst2;
	c = a * b;

	if(neq(a, 3.14159265358979, 1e-12))
		exit(1);
	if(neq(b, 2.71828182845905, 1e-12))
		exit(2);
	if(neq(c, 8.53973422267357, 1e-9))
		exit(3);

	dconst = c;		/* FMOVD store, large SB-offset */
	if(neq(dconst, c, 1e-12))
		exit(4);

	if(padding[0] != 1 || padding[PAD-1] != 2)
		exit(5);	/* the padding array itself got clobbered */

	exit(0);
}
