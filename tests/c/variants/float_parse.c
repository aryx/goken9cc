// Long-mantissa float literals expose the float parsing divergence
// between the two lineages:
//
//  - the kencc cc (compilers/cck/lex.c) uses plan9's mpatof()
//    (compilers/cck/mpatof.c): it builds the digit string as a
//    multi-precision integer, but for literals with digits after the
//    decimal point it computes the result as d1/d2 where d1 and d2
//    are each already rounded to double by mptof(); that division of
//    two rounded values (double rounding) can be off by 1 ulp from
//    the correctly rounded result.
//
//  - the principia cc (compilers/cc/lex.c) uses lib9's strtod()
//    (lib_core/libc/fmt/strtod.c), which rounds the infinitely
//    precise decimal value to nearest-even in one step, per IEEE 754.
//
// Concretely, for the two Hart & Cheney coefficients below (from
// lib_core/libc/port/sin.c, where this was first caught):
//
//   .1357884097877375669092680e8  (little-endian bytes of the double)
//      mpatof: 58 1d 52 1f 4b e6 69 41   (1 ulp too high)
//      strtod: 57 1d 52 1f 4b e6 69 41   == correctly rounded
//   .1459688406665768722226959e3
//      mpatof: 70 3f 24 be 00 3f 62 40   (1 ulp too low)
//      strtod: 71 3f 24 be 00 3f 62 40   == correctly rounded
//
// The principia lineage was the correct one (and matches original
// plan9, whose cc also calls strtod; mpatof came in through the
// Go/inferno fork). Resolved by switching the kencc cc to strtod
// in cck/lex.c.

double
p0(void)
{
	return .1357884097877375669092680e8;
}

double
p4(void)
{
	return .1459688406665768722226959e3;
}
