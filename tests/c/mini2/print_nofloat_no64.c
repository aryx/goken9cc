// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Same as ../float/print.c but without int64 and float stuff

#include "minilibc.h"

// find the position of the terminating null byte ('\0') in a string
int32
findnull(byte *s)
{
	int32 l;

	if(s == nil)
		return 0;
	for(l=0; s[l]!=0; l++)
		;
	return l;
}

void debug(char* s) {
#ifdef DEBUG
    write(2, s, findnull((byte*)s));
    write(2, "\n", 1);
#endif
}


static int32 fd = 1;

void
·printbool(bool v)
{
    debug("PRINTBOOL");

	if(v) {
		write(fd, "true", 4);
		return;
	}
	write(fd, "false", 5);
}

void
prints(int8 *s)
{
    debug("PRINTS");

	write(fd, s, findnull((byte*)s));
}

// for arm64 that does not handle local buf like in printhex below
// and generate a DWORD illegal $buf-100(SP) linking error
byte buf[100];

void
·printhex(uint32 v)
{
	static int8 *dig = "0123456789abcdef";
	//byte buf[100];
	int32 i;

    debug("PRINTHEX");

	i=nelem(buf);
	for(; v>0; v/=16)
		buf[--i] = dig[v%16];
	if(i == nelem(buf))
		buf[--i] = '0';
	buf[--i] = 'x';
	buf[--i] = '0';
	write(fd, (char*) buf+i, nelem(buf)-i);
}

void
·printpointer(void *p)
{
    debug("PRINTPOINTER");

	·printhex((uint32)p);
}

void
·printuint(uint32 v)
{
	//byte buf[100];
	int32 i;

    debug("PRINTUINT");

	for(i=nelem(buf)-1; i>0; i--) {
		buf[i] = v%10 + '0';
		if(v < 10)
			break;
		v = v/10;
	}
	write(fd, (char*) buf+i, nelem(buf)-i);
}

void
·printint(int32 v)
{
    debug("PRINTINT");

	if(v < 0) {
		write(fd, "-", 1);
		v = -v;
	}
	·printuint(v);
}


static byte*
vrnd(byte *p, int32 x)
{
    debug("VRND");

	if((uint32)(uintptr)p&(x-1))
		p += x - ((uint32)(uintptr)p&(x-1));
	return p;
}

// Very simple printf.  Only for debugging prints.
// Do not add to this without checking with Rob.
static void
vprintf(int8 *s, byte *arg)
{
	int8 *p, *lp;
	byte *narg;

    debug("VPRINTF");

	lp = p = s;
	for(; *p; p++) {
		if(*p != '%')
			continue;
		if(p > lp)
			write(fd, lp, p-lp);
		p++;
		narg = nil;
		switch(*p) {
		// claude: %t (bool) needed adapting from the Go ABI to the C ABI.
		// This printf is Rob Pike's Go runtime code (GO/pkg/runtime/
		// print.c). It was written for the Go calling convention, in
		// which a bool is a genuine 1-byte stack argument -- so the
		// original advanced by only 1 byte:
		//old:	case 't':
		//old:		narg = arg + 1;
		//old:		break;
		// (compare the original sibling cases: +1 bool, +4 int32, +8
		// int64 -- each is Go's natural type size.)
		//
		// Here printf is called from C compiled by the plan9 compilers.
		// Under the C ABI, variadic arguments undergo the default
		// argument promotions: a bool/char is widened to int, so it
		// actually occupies a full 4-byte slot -- exactly like %d/%x.
		// There is no portable way to pass a 1-byte value through "..."
		// from C (the promotion is mandatory), so rather than match Rob's
		// 1-byte assumption we adapt the reader to the promoted int: %t
		// now shares the 4-byte, vrnd()-aligned advance with %d/%x. (The
		// vrnd() alignment is itself a C-ABI adaptation already absent
		// from the Go original.)
		case 't':
		case 'd':	// 32-bit
		case 'x':
			arg = vrnd(arg, 4);
			narg = arg + 4;
			break;
		case 'p':	// pointer-sized
		case 's':
			arg = vrnd(arg, sizeof(uintptr));
			narg = arg + sizeof(uintptr);
			break;
		}
		switch(*p) {
		case 'd':
			·printint(*(int32*)arg);
			break;
		case 'p':
			·printpointer(*(void**)arg);
			break;
		case 's':
			prints(*(int8**)arg);
			break;
		case 't':
			// claude: THIS read is the change that actually fixed the
			// bug. Rob's original read the single byte at the lowest
			// address of the arg slot:
			//old:		·printbool(*(bool*)arg);
			// With a real 1-byte Go bool that byte IS the value, on any
			// endianness. But with the C ABI's 4-byte promoted int, the
			// low-address byte is the value's LSB only on little-endian
			// (amd64, arm, arm64) -- which is exactly why those "worked"
			// while nobody noticed. On big-endian mips the low-address
			// byte is the high-order 00, so true (0x00000001) printed as
			// "false". Reading the whole promoted int and testing != 0
			// is endian-safe and matches how %d/%x read their slot.
			·printbool(*(int32*)arg != 0);
			break;
		case 'x':
			·printhex(*(uint32*)arg);
			break;
		case '!':
			panic(-1);
		}
		arg = narg;
		lp = p+1;
	}
	if(p > lp)
		write(fd, lp, p-lp);
}

void
printf(int8 *s, ...)
{
	byte *arg;
    debug("PRINTF");

	arg = (byte*)(&s+1);
	vprintf(s, arg);
}
