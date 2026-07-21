// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Mach-O file writing
// http://developer.apple.com/mac/library/DOCUMENTATION/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html

#include "l.h"
#include "../ld/dwarf.h"
#include "../ld/lib.h"
#include "../ld/macho.h"

static	int	macho64;
static	MachoHdr	hdr;
static	MachoLoad	load[16];
static	MachoSeg	seg[16];
static	MachoDebug	xdebug[16];
static	int	nload, nseg, ndebug, nsect;
static	uint32	machoflags = 1;	/* MH_NOUNDEFS; modern path ORs in DYLDLINK|TWOLEVEL */

/*
 * Dynamic imports declared via "6l -I got:remote:lib": the program defines a
 * GOT pointer slot `got` (8 bytes of zero in __DATA) and calls through it; the
 * linker emits an LC_DYLD_INFO non-lazy bind that makes dyld resolve `remote`
 * from `lib` into that slot at load. Only libSystem (dylib ordinal 1) is wired.
 */
static struct {
	char	*got;
	char	*remote;
	char	*lib;
} dynimp[16];
static int ndynimp;
static uchar	machobind[1024];
static int	nmachobind;
static vlong	machobindoff;

void
adddynimp(char *spec)
{
	char *s, *p, *q;

	if(ndynimp >= nelem(dynimp)) {
		diag("too many -I imports");
		errorexit();
	}
	s = strdup(spec);
	p = utfrune(s, ':');
	q = p ? utfrune(p+1, ':') : nil;
	if(p == nil || q == nil) {
		diag("bad -I import (want got:remote:lib): %s", spec);
		errorexit();
	}
	*p = '\0';
	*q = '\0';
	dynimp[ndynimp].got = s;
	dynimp[ndynimp].remote = p+1;
	dynimp[ndynimp].lib = q+1;
	ndynimp++;
}

static void
binduleb(vlong v)
{
	uchar b;

	do {
		b = v & 0x7f;
		v >>= 7;
		if(v)
			b |= 0x80;
		machobind[nmachobind++] = b;
	} while(v);
}

/*
 * dyld "rebase" opcode stream (LC_DYLD_INFO_ONLY rebase_off/size). A PIE image
 * is slid by ASLR at load, so every __DATA slot holding a link-time-absolute
 * pointer -- i.e. every D_ADDR reloc on a data symbol, e.g. a C global
 * 'static char *dig = "...";' -- must have the slide added by dyld: such a
 * pointer lives in writable data and can not be made pc-relative like a code
 * reference (see the RIP-relative $sym(SB) handling in span.c). This mirrors
 * 7l's linkers/lk/macho.c machorebase(), adapted to 6l's Reloc data model.
 * Only emitted for thechar=='6' (macOS -H6).
 */
static uchar	*machorebasep;	/* opcode stream (malloc'd) */
static int	nmachorebase;
static vlong	machorebaseoff;

static int
rebaseuleb(vlong v, uchar *p)
{
	int n;

	n = 0;
	do {
		p[n] = v & 0x7f;
		v >>= 7;
		if(v)
			p[n] |= 0x80;
		n++;
	} while(v);
	return n;
}

/* datbase is the __DATA segment vmaddr (va+v in asmbmacho) */
static void
machorebase(vlong datbase)
{
	Sym *s;
	Reloc *r;
	vlong *off;
	int noff, moff, i, j;
	vlong o;
	uchar *buf;
	int n, m;

	off = nil;
	noff = 0;
	moff = 0;
	for(s = datap; s != S; s = s->next) {
		for(r = s->r; r < s->r + s->nr; r++) {
			if(r->type != D_ADDR)
				continue;
			if(r->siz != PtrSize) {
				diag("PIE: %d-byte address reloc in data (%s+%d) can not be rebased",
					r->siz, s->name, r->off);
				continue;
			}
			if(noff >= moff) {
				moff = 2*moff + 64;
				off = realloc(off, moff*sizeof off[0]);
			}
			off[noff++] = symaddr(s) + r->off - datbase;
		}
	}
	if(noff == 0)
		return;

	/* dyld wants ascending offsets within the segment */
	for(i = 1; i < noff; i++) {
		o = off[i];
		for(j = i; j > 0 && off[j-1] > o; j--)
			off[j] = off[j-1];
		off[j] = o;
	}

	/* worst case: SET_SEGMENT opcode + 10-byte uleb + DO_REBASE per ptr */
	m = 2 + noff*12 + 8;
	buf = malloc(m);
	n = 0;
	buf[n++] = 0x10 | 1;		/* REBASE_OPCODE_SET_TYPE_IMM | POINTER */
	for(i = 0; i < noff; i++) {
		buf[n++] = 0x20 | 2;	/* SET_SEGMENT_AND_OFFSET_ULEB, seg 2 = __DATA */
		n += rebaseuleb(off[i], buf+n);
		buf[n++] = 0x50 | 1;	/* DO_REBASE_IMM_TIMES, 1 */
	}
	buf[n++] = 0x00;		/* REBASE_OPCODE_DONE */
	while(n & 7)
		buf[n++] = 0x00;	/* pad to 8 bytes */

	machorebasep = buf;
	nmachorebase = n;
	free(off);
}

void
machoinit(void)
{
	switch(thechar) {
	// 64-bit architectures
	case '6':
		macho64 = 1;
		break;

	// 32-bit architectures
	default:
		break;
	}
}

MachoHdr*
getMachoHdr(void)
{
	return &hdr;
}

MachoLoad*
newMachoLoad(uint32 type, uint32 ndata)
{
	MachoLoad *l;

	if(nload >= nelem(load)) {
		diag("too many loads");
		errorexit();
	}
	
	if(macho64 && (ndata & 1))
		ndata++;
	
	l = &load[nload++];
	l->type = type;
	l->ndata = ndata;
	l->data = mal(ndata*4);
	return l;
}

MachoSeg*
newMachoSeg(char *name, int msect)
{
	MachoSeg *s;

	if(nseg >= nelem(seg)) {
		diag("too many segs");
		errorexit();
	}
	s = &seg[nseg++];
	s->name = name;
	s->msect = msect;
	s->sect = mal(msect*sizeof s->sect[0]);
	return s;
}

MachoSect*
newMachoSect(MachoSeg *seg, char *name)
{
	MachoSect *s;

	if(seg->nsect >= seg->msect) {
		diag("too many sects in segment %s", seg->name);
		errorexit();
	}
	s = &seg->sect[seg->nsect++];
	s->name = name;
	nsect++;
	return s;
}

MachoDebug*
newMachoDebug(void)
{
	if(ndebug >= nelem(xdebug)) {
		diag("too many debugs");
		errorexit();
	}
	return &xdebug[ndebug++];
}


// Generic linking code.

static uchar *linkdata;
static uint32 nlinkdata;
static uint32 mlinkdata;

static uchar *strtab;
static uint32 nstrtab;
static uint32 mstrtab;

struct	Expsym
{
	int	off;
	Sym*	s;
} *expsym;
static int nexpsym;
static int nimpsym;

static char **dylib;
static int ndylib;

static vlong linkoff;

int
machowrite(void)
{
	vlong o1;
	int loadsize;
	int i, j;
	MachoSeg *s;
	MachoSect *t;
	MachoDebug *d;
	MachoLoad *l;

	o1 = cpos();

	loadsize = 4*4*ndebug;
	for(i=0; i<nload; i++)
		loadsize += 4*(load[i].ndata+2);
	if(macho64) {
		loadsize += 18*4*nseg;
		loadsize += 20*4*nsect;
	} else {
		loadsize += 14*4*nseg;
		loadsize += 17*4*nsect;
	}

	if(macho64)
		LPUT(0xfeedfacf);
	else
		LPUT(0xfeedface);
	LPUT(hdr.cpu);
	LPUT(hdr.subcpu);
	LPUT(2);	/* file type - mach executable */
	LPUT(nload+nseg+ndebug);
	LPUT(loadsize);
	LPUT(machoflags);	/* flags */
	if(macho64)
		LPUT(0);	/* reserved */

	for(i=0; i<nseg; i++) {
		s = &seg[i];
		if(macho64) {
			LPUT(25);	/* segment 64 */
			LPUT(72+80*s->nsect);
			strnput(s->name, 16);
			VPUT(s->vaddr);
			VPUT(s->vsize);
			VPUT(s->fileoffset);
			VPUT(s->filesize);
			LPUT(s->prot1);
			LPUT(s->prot2);
			LPUT(s->nsect);
			LPUT(s->flag);
		} else {
			LPUT(1);	/* segment 32 */
			LPUT(56+68*s->nsect);
			strnput(s->name, 16);
			LPUT(s->vaddr);
			LPUT(s->vsize);
			LPUT(s->fileoffset);
			LPUT(s->filesize);
			LPUT(s->prot1);
			LPUT(s->prot2);
			LPUT(s->nsect);
			LPUT(s->flag);
		}
		for(j=0; j<s->nsect; j++) {
			t = &s->sect[j];
			if(macho64) {
				strnput(t->name, 16);
				strnput(s->name, 16);
				VPUT(t->addr);
				VPUT(t->size);
				LPUT(t->off);
				LPUT(t->align);
				LPUT(t->reloc);
				LPUT(t->nreloc);
				LPUT(t->flag);
				LPUT(0);	/* reserved */
				LPUT(0);	/* reserved */
				LPUT(0);	/* reserved */
			} else {
				strnput(t->name, 16);
				strnput(s->name, 16);
				LPUT(t->addr);
				LPUT(t->size);
				LPUT(t->off);
				LPUT(t->align);
				LPUT(t->reloc);
				LPUT(t->nreloc);
				LPUT(t->flag);
				LPUT(0);	/* reserved */
				LPUT(0);	/* reserved */
			}
		}
	}

	for(i=0; i<nload; i++) {
		l = &load[i];
		LPUT(l->type);
		LPUT(4*(l->ndata+2));
		for(j=0; j<l->ndata; j++)
			LPUT(l->data[j]);
	}

	for(i=0; i<ndebug; i++) {
		d = &xdebug[i];
		LPUT(3);	/* obsolete gdb debug info */
		LPUT(16);	/* size of symseg command */
		LPUT(d->fileoffset);
		LPUT(d->filesize);
	}

	return cpos() - o1;
}

static void*
grow(uchar **dat, uint32 *ndat, uint32 *mdat, uint32 n)
{
	uchar *p;
	uint32 old;

	if(*ndat+n > *mdat) {
		old = *mdat;
		*mdat = (*ndat+n)*2 + 128;
		*dat = realloc(*dat, *mdat);
		if(*dat == 0) {
			diag("out of memory");
			errorexit();
		}
		memset(*dat+old, 0, *mdat-old);
	}
	p = *dat + *ndat;
	*ndat += n;
	return p;
}

static int
needlib(char *name)
{
	char *p;
	Sym *s;

	/* reuse hash code in symbol table */
	p = smprint(".machoload.%s", name);
	s = lookup(p, 0);
	if(s->type == 0) {
		s->type = 100;	// avoid SDATA, etc.
		return 1;
	}
	return 0;
}

void
domacho(void)
{
	int h, ptrsize, t;
	char *p;
	uchar *dat;
	uint32 x;
	Sym *s;
	Sym **impsym;

	ptrsize = 4;
	if(macho64)
		ptrsize = 8;

	// empirically, string table must begin with " \x00".
	if(!debug['d'])
		*(char*)grow(&strtab, &nstrtab, &mstrtab, 2) = ' ';

	impsym = nil;
	for(h=0; h<NHASH; h++) {
		for(s=hash[h]; s!=S; s=s->hash) {
			if(!s->reachable || (s->type != STEXT && s->type != SDATA && s->type != SBSS) || s->dynimpname == nil)
				continue;
			if(debug['d']) {
				diag("cannot use dynamic loading and -d");
				errorexit();
			}
			if(!s->dynexport) {
				if(nimpsym%32 == 0) {
					impsym = realloc(impsym, (nimpsym+32)*sizeof impsym[0]);
					if(impsym == nil) {
						diag("out of memory");
						errorexit();
					}
				}
				impsym[nimpsym++] = s;
				continue;
			}

			/* symbol table entry - darwin still puts _ prefixes on all C symbols */
			x = nstrtab;
			p = grow(&strtab, &nstrtab, &mstrtab, 1+strlen(s->dynimpname)+1);
			*p++ = '_';
			strcpy(p, s->dynimpname);

			dat = grow(&linkdata, &nlinkdata, &mlinkdata, 8+ptrsize);
			dat[0] = x;
			dat[1] = x>>8;
			dat[2] = x>>16;
			dat[3] = x>>24;

			dat[4] = 0x0f;	// type: N_SECT | N_EXT - external, defined in sect
			switch(s->type) {
			default:
			case STEXT:
				t = 1;
				break;
			case SDATA:
				t = 2;
				break;
			case SBSS:
				t = 4;
				break;
			}
			dat[5] = t;	// sect: section number

			if (nexpsym%32 == 0) {
				expsym = realloc(expsym, (nexpsym+32)*sizeof expsym[0]);
				if (expsym == nil) {
					diag("out of memory");
					errorexit();
				}
			}
			expsym[nexpsym].off = nlinkdata - ptrsize;
			expsym[nexpsym++].s = s;
		}
	}

	for(h=0; h<nimpsym; h++) {
		s = impsym[h];
		s->type = SMACHO;
		s->value = (nexpsym+h) * ptrsize;

		/* symbol table entry - darwin still puts _ prefixes on all C symbols */
		x = nstrtab;
		p = grow(&strtab, &nstrtab, &mstrtab, 1+strlen(s->dynimpname)+1);
		*p++ = '_';
		strcpy(p, s->dynimpname);

		dat = grow(&linkdata, &nlinkdata, &mlinkdata, 8+ptrsize);
		dat[0] = x;
		dat[1] = x>>8;
		dat[2] = x>>16;
		dat[3] = x>>24;

		dat[4] = 0x01;	// type: N_EXT - external symbol

		if(needlib(s->dynimplib)) {
			if(ndylib%32 == 0) {
				dylib = realloc(dylib, (ndylib+32)*sizeof dylib[0]);
				if(dylib == nil) {
					diag("out of memory");
					errorexit();
				}
			}
			dylib[ndylib++] = s->dynimplib;
		}
	}
	free(impsym);

	/*
	 * list of symbol table indexes.
	 * we don't take advantage of the opportunity
	 * to order the symbol table differently from
	 * this list, so it is boring: 0 1 2 3 4 ...
	 */
	for(x=0; x<nexpsym+nimpsym; x++) {
		dat = grow(&linkdata, &nlinkdata, &mlinkdata, 4);
		dat[0] = x;
		dat[1] = x>>8;
		dat[2] = x>>16;
		dat[3] = x>>24;
	}

	dynptrsize = (nexpsym+nimpsym) * ptrsize;
}

vlong
domacholink(void)
{
	int i;
	uchar *p;
	Sym *s;
	uint64 val;

	linkoff = 0;
	if(nlinkdata > 0 || nstrtab > 0) {
		linkoff = rnd(HEADR+textsize, INITRND) + rnd(segdata.filelen - dynptrsize, INITRND);
		seek(cout, linkoff, 0);

		for(i = 0; i<nexpsym; ++i) {
			s = expsym[i].s;
			val = s->value;
			if(s->type == SXREF)
				diag("export of undefined symbol %s", s->name);
			if (s->type != STEXT)
				val += INITDAT;
			p = linkdata+expsym[i].off;
			p[0] = val;
			p[1] = val >> 8;
			p[2] = val >> 16;
			p[3] = val >> 24;
			if (macho64) {
				p[4] = val >> 32;
				p[5] = val >> 40;
				p[6] = val >> 48;
				p[7] = val >> 56;
			}
		}

		ewrite(cout, linkdata, nlinkdata);
		ewrite(cout, strtab, nstrtab);
	}
	return rnd(nlinkdata+nstrtab, INITRND);
}

void
asmbmacho(vlong symdatva, vlong symo)
{
	vlong v, w;
	vlong va;
	vlong entryoff;
	int a, i, ptrsize;
	char *pkgroot;
	MachoHdr *mh;
	MachoSect *msect;
	MachoSeg *ms;
	MachoDebug *md;
	MachoLoad *ml;

	/* apple MACH */
	va = INITTEXT - HEADR;
	mh = getMachoHdr();
	switch(thechar){
	default:
		diag("unknown mach architecture");
		errorexit();
	case '6':
		mh->cpu = MACHO_CPU_AMD64;
		mh->subcpu = MACHO_SUBCPU_X86;
		ptrsize = 8;
		break;
	case '8':
		mh->cpu = MACHO_CPU_386;
		mh->subcpu = MACHO_SUBCPU_X86;
		ptrsize = 4;
		break;
	}

	/* segment for zero page */
	ms = newMachoSeg("__PAGEZERO", 0);
	ms->vsize = va;

	/* text */
	v = rnd(HEADR+textsize, INITRND);
	ms = newMachoSeg("__TEXT", 1);
	ms->vaddr = va;
	ms->vsize = v;
	ms->filesize = v;
	ms->prot1 = 7;
	ms->prot2 = 5;

	msect = newMachoSect(ms, "__text");
	msect->addr = INITTEXT;
	msect->size = textsize;
	msect->off = INITTEXT - va;
	msect->flag = 0x400;	/* flag - some instructions */

	/* data */
	w = segdata.len;
	ms = newMachoSeg("__DATA", 2+(dynptrsize>0));
	ms->vaddr = va+v;
	ms->vsize = w;
	ms->fileoffset = v;
	ms->filesize = segdata.filelen;
	ms->prot1 = 7;
	ms->prot2 = 3;

	msect = newMachoSect(ms, "__data");
	msect->addr = va+v;
	msect->size = segdata.filelen - dynptrsize;
	msect->off = v;

	if(dynptrsize > 0) {
		msect = newMachoSect(ms, "__nl_symbol_ptr");
		msect->addr = va+v+segdata.filelen - dynptrsize;
		msect->size = dynptrsize;
		msect->off = v+segdata.filelen - dynptrsize;
		msect->align = 2;
		msect->flag = 6;	/* section with nonlazy symbol pointers */
		/*
		 * The reserved1 field is supposed to be the index of
		 * the first entry in the list of symbol table indexes
		 * in isymtab for the symbols we need.  We only use
		 * pointers, so we need the entire list, so the index
		 * here should be 0, which luckily is what the Mach-O
		 * writing code emits by default for this not really reserved field.
		msect->reserved1 = 0; - first indirect symbol table entry we need
		 */
	}

	msect = newMachoSect(ms, "__bss");
	msect->addr = va+v+segdata.filelen;
	msect->size = segdata.len - segdata.filelen;
	msect->flag = 1;	/* flag - zero fill */

	switch(thechar) {
	default:
		diag("unknown macho architecture");
		errorexit();
	case '6':
		/*
		 * Modern macOS: LC_MAIN (an entry-point file offset) replaces the
		 * old LC_UNIXTHREAD register-state entry. dyld runs first, then calls
		 * this offset. Also flag the binary as dynamically linked + two-level
		 * (MH_DYLDLINK|MH_TWOLEVEL|MH_PIE) as ld64 does. amd64 uses
		 * RIP-relative addressing for symbol references under -H6 (see
		 * asmandsz), so the image is position-independent and dyld can slide
		 * it under ASLR. __PAGEZERO stays at 1MB (not 4GB) because 6l's text
		 * layout doesn't yet handle __TEXT >= 4GB -- separate work.
		 */
		machoflags = 1 | 4 | 0x80 | 0x200000;	/* MH_NOUNDEFS|MH_DYLDLINK|MH_TWOLEVEL|MH_PIE */
		entryoff = entryvalue() - (INITTEXT - HEADR);	/* file offset of entry */
		ml = newMachoLoad(0x80000028, 4);	/* LC_MAIN */
		ml->data[0] = entryoff;			/* entryoff low */
		ml->data[1] = entryoff>>16>>16;		/* entryoff high */
		ml->data[2] = 0;			/* stacksize low */
		ml->data[3] = 0;			/* stacksize high */
		break;
	case '8':
		ml = newMachoLoad(5, 16+2);	/* unix thread */
		ml->data[0] = 1;	/* thread type */
		ml->data[1] = 16;	/* word count */
		ml->data[2+10] = entryvalue();	/* start pc */
		break;
	}

	if(thechar == '6') {
		/* modern macOS metadata load commands */

		/*
		 * Rebase stream: __DATA pointers dyld must slide (ASLR). Laid out
		 * first in the __LINKEDIT tail slack, right after symtab/strtab; the
		 * bind stream (if any) follows it. Both fit in the page-rounding slack
		 * that domacholink() left before __SYMDAT (see its rnd()).
		 */
		nmachorebase = 0;
		machorebaseoff = 0;
		machorebase(va+v);
		if(nmachorebase > 0)
			machorebaseoff = linkoff + nlinkdata + nstrtab;	/* after symtab/strtab */

		/* build the non-lazy bind opcode stream for -I imports (libSystem, ord 1) */
		nmachobind = 0;
		machobindoff = 0;
		if(ndynimp > 0) {
			char *cp;
			Sym *isym;

			for(i=0; i<ndynimp; i++) {
				isym = lookup(dynimp[i].got, 0);
				machobind[nmachobind++] = 0x10 | 1;	/* SET_DYLIB_ORDINAL_IMM, libSystem */
				machobind[nmachobind++] = 0x40;		/* SET_SYMBOL_TRAILING_FLAGS_IMM, 0 */
				for(cp = dynimp[i].remote; *cp; cp++)
					machobind[nmachobind++] = *cp;
				machobind[nmachobind++] = 0;
				machobind[nmachobind++] = 0x50 | 1;	/* SET_TYPE_IMM, BIND_TYPE_POINTER */
				machobind[nmachobind++] = 0x70 | 2;	/* SET_SEGMENT_AND_OFFSET_ULEB, __DATA */
				binduleb(symaddr(isym) - (va+v));	/* slot offset within __DATA */
				machobind[nmachobind++] = 0x90;		/* DO_BIND */
			}
			machobind[nmachobind++] = 0x00;			/* DONE */
			/* after symtab/strtab and the rebase stream */
			machobindoff = linkoff + nlinkdata + nstrtab + nmachorebase;
		}
		ml = newMachoLoad(0x80000022, 10);	/* LC_DYLD_INFO_ONLY */
		ml->data[0] = machorebaseoff;		/* rebase_off */
		ml->data[1] = nmachorebase;		/* rebase_size */
		ml->data[2] = machobindoff;		/* bind_off */
		ml->data[3] = nmachobind;		/* bind_size */

		ml = newMachoLoad(0x1b, 4);		/* LC_UUID (deterministic placeholder) */
		ml->data[0] = 0x6b636e6b;
		ml->data[1] = 0x36303963;
		ml->data[2] = 0x6f72636d;
		ml->data[3] = 0x00000073;

		ml = newMachoLoad(0x32, 4);		/* LC_BUILD_VERSION */
		ml->data[0] = 1;			/* platform = PLATFORM_MACOS */
		ml->data[1] = 0x000a0f00;		/* minos 10.15.0 */
		ml->data[2] = 0x000a0f00;		/* sdk   10.15.0 */
		ml->data[3] = 0;			/* ntools */

		ml = newMachoLoad(12, 4+(strlen("/usr/lib/libSystem.B.dylib")+1+7)/8*2);	/* LC_LOAD_DYLIB */
		ml->data[0] = 24;			/* name offset */
		ml->data[1] = 2;			/* timestamp */
		ml->data[2] = 0x051f0000;		/* current_version 1311.0.0 */
		ml->data[3] = 0x00010000;		/* compatibility_version 1.0.0 */
		strcpy((char*)&ml->data[4], "/usr/lib/libSystem.B.dylib");
	}

	if(!debug['d']) {
		int nsym;

		nsym = dynptrsize/ptrsize;

		ms = newMachoSeg("__LINKEDIT", 0);
		ms->vaddr = va+v+rnd(segdata.len, INITRND);
		ms->vsize = nlinkdata+nstrtab+nmachorebase+nmachobind;
		ms->fileoffset = linkoff;
		ms->filesize = nlinkdata+nstrtab+nmachorebase+nmachobind;
		ms->prot1 = 7;
		ms->prot2 = 3;

		ml = newMachoLoad(2, 4);	/* LC_SYMTAB */
		ml->data[0] = linkoff;	/* symoff */
		ml->data[1] = nsym;	/* nsyms */
		ml->data[2] = linkoff + nlinkdata;	/* stroff */
		ml->data[3] = nstrtab;	/* strsize */

		ml = newMachoLoad(11, 18);	/* LC_DYSYMTAB */
		ml->data[0] = 0;	/* ilocalsym */
		ml->data[1] = 0;	/* nlocalsym */
		ml->data[2] = 0;	/* iextdefsym */
		ml->data[3] = nexpsym;	/* nextdefsym */
		ml->data[4] = nexpsym;	/* iundefsym */
		ml->data[5] = nimpsym;	/* nundefsym */
		ml->data[6] = 0;	/* tocoffset */
		ml->data[7] = 0;	/* ntoc */
		ml->data[8] = 0;	/* modtaboff */
		ml->data[9] = 0;	/* nmodtab */
		ml->data[10] = 0;	/* extrefsymoff */
		ml->data[11] = 0;	/* nextrefsyms */
		ml->data[12] = linkoff + nlinkdata - nsym*4;	/* indirectsymoff */
		ml->data[13] = nsym;	/* nindirectsyms */
		ml->data[14] = 0;	/* extreloff */
		ml->data[15] = 0;	/* nextrel */
		ml->data[16] = 0;	/* locreloff */
		ml->data[17] = 0;	/* nlocrel */

		ml = newMachoLoad(14, 6);	/* LC_LOAD_DYLINKER */
		ml->data[0] = 12;	/* offset to string */
		strcpy((char*)&ml->data[1], "/usr/lib/dyld");

		if(ndylib > 0) {	/* add reference to where .so files are installed */
			pkgroot = smprint("%s/pkg/%s_%s", goroot, goos, goarch);
			ml = newMachoLoad(0x80000000 | 0x1c, 1+(strlen(pkgroot)+1+7)/8*2);	/* LC_RPATH */
			ml->data[0] = 12;	/* offset of string from beginning of load */
			strcpy((char*)&ml->data[1], pkgroot);
		}
		for(i=0; i<ndylib; i++) {
			ml = newMachoLoad(12, 4+(strlen(dylib[i])+1+7)/8*2);	/* LC_LOAD_DYLIB */
			ml->data[0] = 24;	/* offset of string from beginning of load */
			ml->data[1] = 0;	/* time stamp */
			ml->data[2] = 0;	/* version */
			ml->data[3] = 0;	/* compatibility version */
			strcpy((char*)&ml->data[4], dylib[i]);
		}
	}

	if(!debug['s']) {
		ms = newMachoSeg("__SYMDAT", 1);
		ms->vaddr = symdatva;
		ms->vsize = 8+symsize+lcsize;
		ms->fileoffset = symo;
		ms->filesize = 8+symsize+lcsize;
		ms->prot1 = 7;
		ms->prot2 = 5;

		md = newMachoDebug();
		md->fileoffset = symo+8;
		md->filesize = symsize;

		md = newMachoDebug();
		md->fileoffset = symo+8+symsize;
		md->filesize = lcsize;

		dwarfaddmachoheaders();
	}

	a = machowrite();
	if(a > MACHORESERVE)
		diag("MACHORESERVE too small: %d > %d", a, MACHORESERVE);

	/*
	 * Append the dyld bind opcode stream at the end of __LINKEDIT.
	 * machowrite() buffers the header via LPUT; flush it to disk first so
	 * these direct seek/ewrite calls don't race the buffered output.
	 */
	if(nmachorebase > 0) {
		cflush();
		seek(cout, machorebaseoff, 0);
		ewrite(cout, machorebasep, nmachorebase);
	}
	if(nmachobind > 0) {
		cflush();
		seek(cout, machobindoff, 0);
		ewrite(cout, machobind, nmachobind);
	}
}
