// Regression test for two related 64-bit-constant truncation bugs, both
// instances of this project's recurring long-vs-int32-on-a-64-bit-host
// pattern:
//  - include/6.out.h's Ieee.l/.h fields were `long`; the linkers copy
//    the raw bytes of this struct straight into the data segment
//    (linkers/6l/obj.c, AFMOVD/AMOVSD case), so on our 64-bit hosts
//    every IEEE double literal got corrupted. Already correct in the
//    shared include/common.out.h (used by 5l/8l) and in the older
//    linkers/8lk/6.out.h copy -- this freshly-imported vanilla header
//    just still had the original 32-bit-host `long`.
//  - compilers/6c/swt.c's zaddr() used `long l` for its "does this
//    offset fit in 32 bits" check (l = a->offset; if((vlong)l !=
//    a->offset) t |= T_64;), which can never be true when long is 64
//    bits, so the T_64 object-file flag was never set and any integer
//    constant wider than 32 bits got truncated when linkers/6l/obj.c's
//    zaddr() read it back. Go-era's GO/cmd/6g/gobj.c zaddr() already
//    uses int32 here for exactly this reason.
//
// Minimal repro: a uint64 constant and an IEEE double literal whose
// high and low 32-bit halves are both nonzero and distinguishable.

typedef unsigned long long uint64;
typedef double float64;

extern int write(int fd, char *buf, int count);
extern void exit(int code);

char hex[] = "0123456789abcdef";

void
printhex(uint64 x)
{
	char buf[16];
	int i;

	for(i = 0; i < 16; i++)
		buf[i] = hex[(x >> (60 - i*4)) & 0xf];
	write(1, buf, 16);
	write(1, "\n", 1);
}

uint64
tobits(float64 f)
{
	union {
		float64 f;
		uint64 i;
	} u;

	u.f = f;
	return u.i;
}

void
main(void)
{
	printhex(0x7FF0000000000001ULL);
	printhex(tobits(2.2));
	exit(0);
}
