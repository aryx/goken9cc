// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// code copy-pasted from GO/src/pkg/runtime/

//#include "runtime.h"
//#include "type.h"
#include "minilibc.h"

static int32 fd = 1;

// forward decls
void	·printpointer(void*);
void	·printbool(bool);
void	·printfloat(float64);
void	·printint(int64);
void	·printuint(uint64);
void	·printhex(uint64);

void prints(int8 *s);

static void vprintf(int8*, byte*);

void
prints(int8 *s)
{
	write(fd, s, findnull((byte*)s));
}

//?? for NOSPLIT | DUPOK | NOPROF?
#pragma textflag 7
void
printf(int8 *s, ...)
{
	byte *arg;

	arg = (byte*)(&s+1);
	vprintf(s, arg);
}

static byte*
vrnd(byte *p, int32 x)
{
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

	lp = p = s;
	for(; *p; p++) {
		if(*p != '%')
			continue;
		if(p > lp)
			write(fd, lp, p-lp);
		p++;
		narg = nil;
		switch(*p) {
		case 't':
			narg = arg + 1;
			break;
		case 'd':	// 32-bit
		case 'x':
			arg = vrnd(arg, 4);
			narg = arg + 4;
			break;
		case 'D':	// 64-bit
		case 'U':
		case 'X':
		case 'f':
			arg = vrnd(arg, sizeof(uintptr));
			narg = arg + 8;
			break;
		case 'C':
			arg = vrnd(arg, sizeof(uintptr));
			narg = arg + 16;
			break;
		case 'p':	// pointer-sized
		case 's':
			arg = vrnd(arg, sizeof(uintptr));
			narg = arg + sizeof(uintptr);
			break;
		//case 'S':	// pointer-aligned but bigger
		//	arg = vrnd(arg, sizeof(uintptr));
		//	narg = arg + sizeof(String);
		//	break;
		//case 'a':	// pointer-aligned but bigger
		//	arg = vrnd(arg, sizeof(uintptr));
		//	narg = arg + sizeof(Slice);
		//	break;
		//case 'i':	// pointer-aligned but bigger
		//case 'e':
		//	arg = vrnd(arg, sizeof(uintptr));
		//	narg = arg + sizeof(Eface);
		//	break;
		}
		switch(*p) {
		//case 'a':
		//	·printslice(*(Slice*)arg);
		//	break;
		case 'd':
			·printint(*(int32*)arg);
			break;
		case 'D':
			·printint(*(int64*)arg);
			break;
		//case 'e':
		//	·printeface(*(Eface*)arg);
		//	break;
		case 'f':
			·printfloat(*(float64*)arg);
			break;
		//case 'C':
		//	·printcomplex(*(Complex128*)arg);
		//	break;
		//case 'i':
		//	·printiface(*(Iface*)arg);
		//	break;
		case 'p':
			·printpointer(*(void**)arg);
			break;
		case 's':
			prints(*(int8**)arg);
			break;
		//case 'S':
		//	·printstring(*(String*)arg);
		//	break;
		case 't':
			·printbool(*(bool*)arg);
			break;
		case 'U':
			·printuint(*(uint64*)arg);
			break;
		case 'x':
			·printhex(*(uint32*)arg);
			break;
		case 'X':
			·printhex(*(uint64*)arg);
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

#pragma textflag 7

void
·printbool(bool v)
{
	if(v) {
		write(fd, "true", 4);
		return;
	}
	write(fd, "false", 5);
}

void
·printfloat(float64 v)
{
	byte buf[20];
	int32 e, s, i, n;
	float64 h;

	if(isNaN(v)) {
		write(fd, "NaN", 3);
		return;
	}
	if(isInf(v, 1)) {
		write(fd, "+Inf", 4);
		return;
	}
	if(isInf(v, -1)) {
		write(fd, "-Inf", 4);
		return;
	}


	n = 7;	// digits printed
	e = 0;	// exp
	s = 0;	// sign
	if(v != 0) {
		// sign
		if(v < 0) {
			v = -v;
			s = 1;
		}

		// normalize
		while(v >= 10) {
			e++;
			v /= 10;
		}
		while(v < 1) {
			e--;
			v *= 10;
		}

		// round
		h = 5;
		for(i=0; i<n; i++)
			h /= 10;
		v += h;
		if(v >= 10) {
			e++;
			v /= 10;
		}
	}

	// format +d.dddd+edd
	buf[0] = '+';
	if(s)
		buf[0] = '-';
	for(i=0; i<n; i++) {
		s = v;
		buf[i+2] = s+'0';
		v -= s;
		v *= 10.;
	}
	buf[1] = buf[2];
	buf[2] = '.';

	buf[n+2] = 'e';
	buf[n+3] = '+';
	if(e < 0) {
		e = -e;
		buf[n+3] = '-';
	}

	buf[n+4] = (e/100) + '0';
	buf[n+5] = (e/10)%10 + '0';
	buf[n+6] = (e%10) + '0';
	write(fd, (char*) buf, n+7);
}

void
·printuint(uint64 v)
{
	byte buf[100];
	int32 i;

	for(i=nelem(buf)-1; i>0; i--) {
		buf[i] = v%10 + '0';
		if(v < 10)
			break;
		v = v/10;
	}
	write(fd, (char*) buf+i, nelem(buf)-i);
}

void
·printint(int64 v)
{
	if(v < 0) {
		write(fd, "-", 1);
		v = -v;
	}
	·printuint(v);
}

void
·printhex(uint64 v)
{
	static int8 *dig = "0123456789abcdef";
	byte buf[100];
	int32 i;

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
	·printhex((uint64)p);
}

void
·printsp(void)
{
	write(fd, " ", 1);
}

void
·printnl(void)
{
	write(fd, "\n", 1);
}
