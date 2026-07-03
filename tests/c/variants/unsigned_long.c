// Multi-token type declarations go through simplec() with no class bits
// (b == 0), which must yield CXXX. A literate-programming reorg in the
// principia cc (principia commit 46fc889d, 2016) left "case 0:" falling
// into "case BAUTO: return CAUTO;", so every "unsigned long x;" style
// declaration failed with "illegal combination of class 4: AUTO".
// Single-token types never call simplec(), which is why toy tests passed.

unsigned long
f(unsigned long x)
{
	unsigned long n;
	unsigned char c;
	long int li;

	n = x;
	c = (unsigned char)x;
	li = (long int)c;
	return n + li;
}
