// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "go.h"

/*
 * architecture-independent object file output
 */

static	void	outhist(Biobuf *b);
static	void	dumpglobls(void);

void
dumpobj(void)
{
	bout = Bopen(outfile, OWRITE);
	if(bout == nil) {
		flusherrors();
		print("can't create %s: %r\n", outfile);
		errorexit();
	}

	Bprint(bout, "%s\n", thestring);
	Bprint(bout, "  exports automatically generated from\n");
	Bprint(bout, "  %s in package \"%s\"\n", curio.infile, localpkg->name);
	dumpexport();
	Bprint(bout, "\n!\n");

	outhist(bout);

	// add nil plist w AEND to catch
	// auto-generated trampolines, data
	newplist();

	dumpglobls();
	dumptypestructs();
	dumpdata();
	dumpfuncs();

	Bterm(bout);
}

static void
dumpglobls(void)
{
	Node *n;
	NodeList *l;

	// add globals
	for(l=externdcl; l; l=l->next) {
		n = l->n;
		if(n->op != ONAME)
			continue;

		if(n->type == T)
			fatal("external %#N nil type\n", n);
		if(n->class == PFUNC)
			continue;
		if(n->sym->pkg != localpkg)
			continue;
		dowidth(n->type);

		ggloblnod(n, n->type->width);
	}
}

void
Bputname(Biobuf *b, Sym *s)
{
	Bprint(b, "%s", s->pkg->prefix);
	Bputc(b, '.');
	Bwrite(b, s->name, strlen(s->name)+1);
}

static void
outzfile(Biobuf *b, char *p)
{
	char *q, *q2;

	while(p) {
		q = utfrune(p, '/');
		if(windows) {
			q2 = utfrune(p, '\\');
			if(q2 && (!q || q2 < q))
				q = q2;
		}
		if(!q) {
			zfile(b, p, strlen(p));
			return;
		}
		if(q > p)
			zfile(b, p, q-p);
		p = q + 1;
	}
}

#define isdelim(c) (c == '/' || c == '\\')

static void
outwinname(Biobuf *b, Hist *h, char *ds, char *p)
{
	if(isdelim(p[0])) {
		// full rooted name
		zfile(b, ds, 3);	// leading "c:/"
		outzfile(b, p+1);
	} else {
		// relative name
		if(h->offset == 0 && pathname && pathname[1] == ':') {
			if(tolowerrune(ds[0]) == tolowerrune(pathname[0])) {
				// using current drive
				zfile(b, pathname, 3);	// leading "c:/"
				outzfile(b, pathname+3);
			} else {
				// using drive other then current,
				// we don't have any simple way to
				// determine current working directory
				// there, therefore will output name as is
				zfile(b, ds, 2);	// leading "c:"
			}
		}
		outzfile(b, p);
	}
}

static void
outhist(Biobuf *b)
{
	Hist *h;
	char *p, ds[] = {'c', ':', '/', 0};

	for(h = hist; h != H; h = h->link) {
		p = h->name;
		if(p) {
			if(windows) {
				// if windows variable is set, then, we know already,
				// pathname is started with windows drive specifier
				// and all '\' were replaced with '/' (see lex.c)
				if(isdelim(p[0]) && isdelim(p[1])) {
					// file name has network name in it, 
					// like \\server\share\dir\file.go
					zfile(b, "//", 2);	// leading "//"
					outzfile(b, p+2);
				} else if(p[1] == ':') {
					// file name has drive letter in it
					ds[0] = p[0];
					outwinname(b, h, ds, p+2);
				} else {
					// no drive letter in file name
					outwinname(b, h, pathname, p);
				}
			} else {
				if(p[0] == '/') {
					// full rooted name, like /home/rsc/dir/file.go
					zfile(b, "/", 1);	// leading "/"
					outzfile(b, p+1);
				} else {
					// relative name, like dir/file.go
					if(h->offset == 0 && pathname && pathname[0] == '/') {
						zfile(b, "/", 1);	// leading "/"
						outzfile(b, pathname+1);
					}
					outzfile(b, p);
				}
			}
		}
		zhist(b, h->line, h->offset);
	}
}

void
ieeedtod(uint64 *ieee, double native)
{
	double fr, ho, f;
	int exp;
	uint32 h, l;
	uint64 bits;

	if(native < 0) {
		ieeedtod(ieee, -native);
		*ieee |= 1ULL<<63;
		return;
	}
	if(native == 0) {
		*ieee = 0;
		return;
	}
	fr = frexp(native, &exp);
	f = 2097152L;		/* shouldnt use fp constants here */
	fr = modf(fr*f, &ho);
	h = ho;
	h &= 0xfffffL;
	f = 65536L;
	fr = modf(fr*f, &ho);
	l = ho;
	l <<= 16;
	l |= (int32)(fr*f);
	bits = ((uint64)h<<32) | l;
	if(exp < -1021) {
		// gradual underflow
		bits |= 1LL<<52;
		bits >>= -1021 - exp;
		exp = -1022;
	}
	bits |= (uint64)(exp+1022L) << 52;
	*ieee = bits;
}

int
duint8(Sym *s, int off, uint8 v)
{
	return duintxx(s, off, v, 1);
}

int
duint16(Sym *s, int off, uint16 v)
{
	return duintxx(s, off, v, 2);
}

int
duint32(Sym *s, int off, uint32 v)
{
	return duintxx(s, off, v, 4);
}

int
duint64(Sym *s, int off, uint64 v)
{
	return duintxx(s, off, v, 8);
}

int
duintptr(Sym *s, int off, uint64 v)
{
	return duintxx(s, off, v, widthptr);
}
