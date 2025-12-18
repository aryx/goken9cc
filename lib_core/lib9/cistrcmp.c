#include <u.h>
#include <libc.h>

//defined like this in GO/gc/subr.c: better?
///* unicode-aware case-insensitive strcmp */
//
//static int
//cistrcmp(char *p, char *q)
//{
//	Rune rp, rq;
//
//	while(*p || *q) {
//		if(*p == 0)
//			return +1;
//		if(*q == 0)
//			return -1;
//		p += chartorune(&rp, p);
//		q += chartorune(&rq, q);
//		rp = tolowerrune(rp);
//		rq = tolowerrune(rq);
//		if(rp < rq)
//			return -1;
//		if(rp > rq)
//			return +1;
//	}
//	return 0;
//}


int
cistrcmp(char *s1, char *s2)
{
	int c1, c2;

	while(*s1){
		c1 = *(uchar*)s1++;
		c2 = *(uchar*)s2++;

		if(c1 == c2)
			continue;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	return -*s2;
}
