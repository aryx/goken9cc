// from compilers/8c/mul.c
// the compilation of this file with gcc on arm64 leads to some weird
// runtime behavior with the wrong value returned compared to
// compiling the same file with gcc on amd64 with gcc -m32.

//#include "gc.h"
#include <stdio.h>
typedef unsigned long ulong;
#define print printf
#define nelem(x)    (sizeof(x)/sizeof((x)[0]))



typedef struct	Malg	Malg;
typedef struct	Mparam	Mparam;

struct	Malg
{
	char	vals[10];
};

struct	Mparam
{
	ulong	value;
	char	alg;
	char	neg;
	char	shift;
	char	arg;
	char	off;
};

static	Mparam	multab[32];
static	int	mulptr;

static	Malg	malgs[]	=
{
	{0, 100},
	{-1, 1, 100},
	{-9, -5, -3, 3, 5, 9, 100},
	{6, 10, 12, 18, 20, 24, 36, 40, 72, 100},
	{-8, -4, -2, 2, 4, 8, 100},
};

/*
 * return position of lowest 1
 */
int
lowbit(ulong v)
{
	int s, i;
	ulong m;

	s = 0;
	m = 0xFFFFFFFFUL;
	for(i = 16; i > 0; i >>= 1) {
		m >>= i;
		if((v & m) == 0) {
			v >>= i;
			s += i;
		}
	}
    print("lowbit(%d) = %d\n", v, s);
	return s;
}

void
mulparam(ulong m, Mparam *mp)
{
	int c, i, j, n, o, q, s;
	int bc, bi, bn, bo, bq, bs, bt;
	char *p;
	long u;
	ulong t;
    print("mulparam %d\n", m);


	bc = bq = 10;
	bi = bn = bo = bs = bt = 0;
	for(i = 0; i < nelem(malgs); i++) {
		for(p = malgs[i].vals, j = 0; (o = p[j]) < 100; j++) {
         print("j = %d, o = %d\n", j, o);
		for(s = 0; s < 2; s++) {
			c = 10;
			q = 10;

            print("X: i=%d,j=%d,s=%d, u=%d, t=%d\n", i, j, s, u, t);
            print("Y: m=%d,o=%d\n", m, o);

			u = m - o;
			if(u == 0)
				continue;
			if(s) {
				o = -o;
				if(o > 0)
					continue;
				u = -u;
			}
			n = lowbit(u);
			t = (ulong)u >> n;
            print("i=%d,j=%d,s=%d,u=%d,t=%d\n\n", i, j, s, u, t);
			switch(i) {
			case 0:
				if(t == 1) {
					c = s + 1;
					q = 0;
					break;
				}
				switch(t) {
				case 3:
				case 5:
				case 9:
					c = s + 1;
					if(n)
						c++;
					q = 0;
					break;
				}
				if(s)
					break;
				switch(t) {
				case 15:
				case 25:
				case 27:
				case 45:
				case 81:
					c = 2;
					if(n)
						c++;
					q = 1;
					break;
				}
				break;
			case 1:
				if(t == 1) {
					c = 3;
					q = 3;
					break;
				}
				switch(t) {
				case 3:
				case 5:
				case 9:
					c = 3;
					q = 2;
					break;
				}
				break;
			case 2:
				if(t == 1) {
					c = 3;
					q = 2;
					break;
				}
				break;
			case 3:
				if(s)
					break;
				if(t == 1) {
					c = 3;
					q = 1;
					break;
				}
				break;
			case 4:
				if(t == 1) {
					c = 3;
					q = 0;
					break;
				}
				break;
			}
			if(c < bc || (c == bc && q > bq)) {
				bc = c;
				bi = i;
				bn = n;
				bo = o;
				bq = q;
				bs = s;
				bt = t;
			}
		}
      }
	}
    print("BC = %d, alg = %d\n", bc, bi);
	//mp->value = m;
	//if(bc <= 3) {
	//	mp->alg = bi;
	//	mp->shift = bn;
	//	mp->off = bo;
	//	mp->neg = bs;
	//	mp->arg = bt;
	//}
	//else
	//	mp->alg = -1;
}


void main() {
    mulparam(129, NULL);
}
