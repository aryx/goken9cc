// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Same as ./print.c but without int64 and float stuff

#include "minilibc.h"

static int32 fd = 1;

// forward decls
void	·printpointer(void*);
void	·printbool(bool);
void	·printint(int32);
void	·printpointer(void*);
void	·printuint(uint32);
void	·printhex(uint32);

void prints(int8 *s);

static void vprintf(int8*, byte*);

void
dump(byte *p, int32 n)
{
	int32 i;

	for(i=0; i<n; i++) {
		·printpointer((byte*)(p[i]>>4));
		·printpointer((byte*)(p[i]&0xf));
		if((i&15) == 15)
			prints("\n");
		else
			prints(" ");
	}
	if(n & 15)
		prints("\n");
}


void
prints(int8 *s)
{
	write(fd, s, findnull((byte*)s));
}

// need NOSPLIT otherwise 5l will complain because func uses '...' arg
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
			·printbool(*(bool*)arg);
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
·printbool(bool v)
{
	if(v) {
		write(fd, "true", 4);
		return;
	}
	write(fd, "false", 5);
}

void
·printuint(uint32 v)
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
·printint(int32 v)
{
	if(v < 0) {
		write(fd, "-", 1);
		v = -v;
	}
	·printuint(v);
}

void
·printhex(uint32 v)
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
	·printhex((uint32)p);
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
