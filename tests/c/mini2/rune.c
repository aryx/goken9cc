/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 *              Portions Copyright 2009 The Go Authors. All rights reserved.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/*
 * This code is copied, with slight editing due to type differences,
 * from a subset of ../lib9/utf/rune.c
 */

#include "minilibc.h"

enum
{
	Bit1	= 7,
	Bitx	= 6,
	Bit2	= 5,
	Bit3	= 4,
	Bit4	= 3,
	Bit5	= 2,

	T1	= ((1<<(Bit1+1))-1) ^ 0xFF,	/* 0000 0000 */
	Tx	= ((1<<(Bitx+1))-1) ^ 0xFF,	/* 1000 0000 */
	T2	= ((1<<(Bit2+1))-1) ^ 0xFF,	/* 1100 0000 */
	T3	= ((1<<(Bit3+1))-1) ^ 0xFF,	/* 1110 0000 */
	T4	= ((1<<(Bit4+1))-1) ^ 0xFF,	/* 1111 0000 */
	T5	= ((1<<(Bit5+1))-1) ^ 0xFF,	/* 1111 1000 */

	Rune1	= (1<<(Bit1+0*Bitx))-1,		/* 0000 0000 0111 1111 */
	Rune2	= (1<<(Bit2+1*Bitx))-1,		/* 0000 0111 1111 1111 */
	Rune3	= (1<<(Bit3+2*Bitx))-1,		/* 1111 1111 1111 1111 */
	Rune4	= (1<<(Bit4+3*Bitx))-1,
                                        /* 0001 1111 1111 1111 1111 1111 */

	Maskx	= (1<<Bitx)-1,			/* 0011 1111 */
	Testx	= Maskx ^ 0xFF,			/* 1100 0000 */

	Runeerror	= 0xFFFD,
	Runeself	= 0x80,

	Bad	= Runeerror,

	Runemax	= 0x10FFFF,	/* maximum rune value */
};

/*
 * Modified by Wei-Hwa Huang, Google Inc., on 2004-09-24
 * This is a slower but "safe" version of the old chartorune
 * that works on strings that are not necessarily null-terminated.
 *
 * If you know for sure that your string is null-terminated,
 * chartorune will be a bit faster.
 *
 * It is guaranteed not to attempt to access "length"
 * past the incoming pointer.  This is to avoid
 * possible access violations.  If the string appears to be
 * well-formed but incomplete (i.e., to get the whole Rune
 * we'd need to read past str+length) then we'll set the Rune
 * to Bad and return 0.
 *
 * Note that if we have decoding problems for other
 * reasons, we return 1 instead of 0.
 */
int32
charntorune(int32 *rune, uint8 *str, int32 length)
{
	int32 c, c1, c2, c3, l;

	/* When we're not allowed to read anything */
	if(length <= 0) {
		goto badlen;
	}

	/*
	 * one character sequence (7-bit value)
	 *	00000-0007F => T1
	 */
	c = *(uint8*)str;
	if(c < Tx) {
		*rune = c;
		return 1;
	}

	// If we can't read more than one character we must stop
	if(length <= 1) {
		goto badlen;
	}

	/*
	 * two character sequence (11-bit value)
	 *	0080-07FF => T2 Tx
	 */
	c1 = *(uint8*)(str+1) ^ Tx;
	if(c1 & Testx)
		goto bad;
	if(c < T3) {
		if(c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if(l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	// If we can't read more than two characters we must stop
	if(length <= 2) {
		goto badlen;
	}

	/*
	 * three character sequence (16-bit value)
	 *	0800-FFFF => T3 Tx Tx
	 */
	c2 = *(uint8*)(str+2) ^ Tx;
	if(c2 & Testx)
		goto bad;
	if(c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(l <= Rune2)
			goto bad;
		*rune = l;
		return 3;
	}

	if (length <= 3)
		goto badlen;

	/*
	 * four character sequence (21-bit value)
	 *	10000-1FFFFF => T4 Tx Tx Tx
	 */
	c3 = *(uint8*)(str+3) ^ Tx;
	if (c3 & Testx)
		goto bad;
	if (c < T5) {
		l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
		if (l <= Rune3 || l > Runemax)
			goto bad;
		*rune = l;
		return 4;
	}

	// Support for 5-byte or longer UTF-8 would go here, but
	// since we don't have that, we'll just fall through to bad.

	/*
	 * bad decoding
	 */
bad:
	*rune = Bad;
	return 1;
badlen:
	// was return 0, but return 1 is more convenient for the runtime.
	*rune = Bad;
	return 1;

}

int32
runetochar(byte *str, int32 rune)  /* note: in original, arg2 was pointer */
{
	/* Runes are signed, so convert to unsigned for range check. */
	uint32 c;

	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	c = rune;
	if(c <= Rune1) {
		str[0] = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if(c <= Rune2) {
		str[0] = T2 | (c >> 1*Bitx);
		str[1] = Tx | (c & Maskx);
		return 2;
	}

	/*
	 * If the Rune is out of range, convert it to the error rune.
	 * Do this test here because the error rune encodes to three bytes.
	 * Doing it earlier would duplicate work, since an out of range
	 * Rune wouldn't have fit in one or two bytes.
	 */
	if (c > Runemax)
		c = Runeerror;

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	if (c <= Rune3) {
		str[0] = T3 |  (c >> 2*Bitx);
		str[1] = Tx | ((c >> 1*Bitx) & Maskx);
		str[2] = Tx |  (c & Maskx);
		return 3;
	}

	/*
	 * four character sequence (21-bit value)
	 *     10000-1FFFFF => T4 Tx Tx Tx
	 */
	str[0] = T4 | (c >> 3*Bitx);
	str[1] = Tx | ((c >> 2*Bitx) & Maskx);
	str[2] = Tx | ((c >> 1*Bitx) & Maskx);
	str[3] = Tx | (c & Maskx);
	return 4;
}
