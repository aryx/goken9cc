// Long-mantissa float literals (like the Hart & Cheney coefficients in
// libc/port/sin.c) expose the float parsing difference between the two
// lineages: the kencc cc uses plan9's mpatof() (1 ulp off on hard
// cases), the principia cc uses lib9's strtod() (correctly rounded).
// The principia lineage is the more correct one here; this test tracks
// the divergence until the lineages are aligned (see the mkfile).

double
f(double x)
{
	return x * .1357884097877375669092680e8
	         / .8644558652922534429915149e7;
}
