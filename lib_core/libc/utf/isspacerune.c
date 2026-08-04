/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include <u.h>
#include <libc.h>

/* isspacerune() (include/utf/utf.h), found blocking utilities/files/
 * wc.c. Its real body lives inline in utf/runetypebody-5.0.0.c
 * (generated from UnicodeData-5.0.0.txt, #include'd by utf/
 * runetype.c), but that whole file is 1361 lines covering every
 * Unicode character class (isalpharune, isdigitrune, isupperrune,
 * ...) -- deliberately NOT wired into UTFOFILES for the same reason
 * this mkfile's own UTFOFILES comment already gives ("the 1400+ line
 * Unicode runetype tables nothing here needs"). Extracted just
 * isspacerune()'s own __isspacer table and rbsearch() helper (also
 * from runetypebody-5.0.0.c/runetype.c) into this standalone file
 * instead of wiring in the whole thing for one function.
 */

static Rune*
rbsearch(Rune c, Rune *t, int n, int ne)
{
	Rune *p;
	int m;

	while(n > 1) {
		m = n >> 1;
		p = t + m*ne;
		if(c >= p[0]) {
			t = p;
			n = n-m;
		} else
			n = m;
	}
	if(n && c >= t[0])
		return t;
	return 0;
}

static Rune __isspacer[] = {
	0x0009, 0x000d,
	0x0020, 0x0020,
	0x0085, 0x0085,
	0x00a0, 0x00a0,
	0x1680, 0x1680,
	0x180e, 0x180e,
	0x2000, 0x200a,
	0x2028, 0x2029,
	0x202f, 0x202f,
	0x205f, 0x205f,
	0x3000, 0x3000,
	0xfeff, 0xfeff,
};

int
isspacerune(Rune c)
{
	Rune *p;

	p = rbsearch(c, __isspacer, nelem(__isspacer)/2, 2);
	if(p && c >= p[0] && c <= p[1])
		return 1;
	return 0;
}
