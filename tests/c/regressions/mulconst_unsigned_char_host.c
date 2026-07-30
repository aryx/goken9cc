// mulconst_unsigned_char_host: found while bringing up lib_core/libc's
// amd64/386 build on this project's own arm64-hosted boot-gcc (the
// cross-compilers 6c/8c/8ck are themselves compiled by the *host's*
// gcc -- see docs/claude_notes/notes_libc_selfhost.txt). Reproduces
// `x = A*lo - R*hi;` from lib_core/libc/math/lrand.c's isrand()
// (A=48271, R=3399), which failed to compile at all on this host with
// "bad mul alg" from both compilers/6c/mul.c and compilers/8c/mul.c
// (and compilers/8ck/mul.c, same bug independently).
//
// Root cause: compilers/{6c,8c,8ck}/mul.c's constant-multiply
// strength-reduction search (mulgen1()/mulparam()) stores its "no cheap
// algorithm found, caller should fall back to a real MUL instruction"
// sentinel as `mp->alg = -1` in a field declared plain `char alg;`, and
// separately looks up shift-add patterns in a table (Malg.vals[10],
// also plain `char`) containing negative entries like -9/-5/-3/-1/-8/
// -4/-2. Plain `char`'s signedness is implementation-defined; it
// defaults to *unsigned* on arm64/AAPCS hosts (unlike x86, where gcc
// defaults it to signed) -- so on this project's own arm64-hosted
// boot-gcc, `-1` read back out of `alg` as 255, `if(p->alg < 0)` (the
// intended fallback check) was never true, and execution fell into
// `switch(p->alg)`'s unhandled `default:`, diag()ing "bad mul alg"
// instead of silently emitting a real MUL. The table's negative
// entries had the same problem independently (silently disabling three
// of the five algorithm rows, a *correctness-preserving* but
// undetected loss of optimization, not itself a compile failure).
//
// Fixed by making Malg.vals, Mparam.alg, and Mparam.off (the three
// fields that legitimately carry negative values) explicitly `signed
// char` in all three compilers, instead of relying on the host's
// default char signedness.
//
// Confirmed real before the fix: "6c -c mulconst_unsigned_char_host.c"
// (and 8c, 8ck) exited 1 with "bad mul alg" on this arm64-hosted
// boot-gcc build, for the exact A/R constants below.
//
// Verify by hand:
//   6c -c mulconst_unsigned_char_host.c
//   6a -c mulzero_start_amd64.s
//   6l -o mulconst_unsigned_char_host_amd64.exe -E _main \
//      mulconst_unsigned_char_host.6 mulzero_start_amd64.6
//   ../../../scripts/qemu-runner amd64 ./mulconst_unsigned_char_host_amd64.exe; echo $?
// (want 0; mulzero_start_{amd64,386}.s -- already-existing, bug-generic
// _main/exit stubs -- are reused here rather than adding new ones)

extern void exit(int);

void
main(void)
{
	long lo, hi, x;

	lo = 1;
	hi = 1;
	x = 48271*lo - 3399*hi;	/* A*lo - R*hi, lib_core/libc/math/lrand.c */
	if (x != 44872)
		exit(1);
	exit(0);
}
