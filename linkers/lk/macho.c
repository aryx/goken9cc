// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
 * Mach-O file writing, for ?l targeting macOS.
 * this is a component of ?l (only 7l for now).
 *
 * Adapted from src/cmd/ld/macho.c but simplified: 64-bit only,
 * static executables only (no dylibs, no symbol table), and
 * using the ?l primitives (lputl, llputl, ...) instead of the
 * newer src/cmd/ld/ ones.
 * http://developer.apple.com/mac/library/DOCUMENTATION/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html
 */
#include "l.h"

static	MachoHdr	hdr;
static	MachoLoad	load[16];
static	MachoSeg	seg[16];
static	int	nload, nseg, nsect;

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

	/* load commands must be 8-byte aligned in 64-bit Mach-O */
	if(ndata & 1)
		ndata++;

	l = &load[nload++];
	l->type = type;
	l->ndata = ndata;
	l->data = halloc(ndata*4);
	memset(l->data, 0, ndata*4);
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
	s->sect = halloc(msect*sizeof s->sect[0]);
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

/* returns the total size of the header and load commands */
int
machowrite(void)
{
	int loadsize;
	int i, j;
	MachoSeg *s;
	MachoSect *t;
	MachoLoad *l;

	loadsize = 0;
	for(i=0; i<nload; i++)
		loadsize += 4*(load[i].ndata+2);
	loadsize += 18*4*nseg;
	loadsize += 20*4*nsect;

	lputl(0xfeedfacf);	/* 64-bit magic */
	lputl(hdr.cpu);
	lputl(hdr.subcpu);
	lputl(MACHO_EXECUTE);	/* file type - mach executable */
	lputl(nload+nseg);
	lputl(loadsize);
	lputl(hdr.flags);
	lputl(0);	/* reserved */

	for(i=0; i<nseg; i++) {
		s = &seg[i];
		lputl(MACHO_SEGMENT_64);
		lputl(72+80*s->nsect);
		strnput(s->name, 16);
		llputl(s->vaddr);
		llputl(s->vsize);
		llputl(s->fileoffset);
		llputl(s->filesize);
		lputl(s->prot1);
		lputl(s->prot2);
		lputl(s->nsect);
		lputl(s->flag);

		for(j=0; j<s->nsect; j++) {
			t = &s->sect[j];
			strnput(t->name, 16);
			strnput(s->name, 16);
			llputl(t->addr);
			llputl(t->size);
			lputl(t->off);
			lputl(t->align);
			lputl(t->reloc);
			lputl(t->nreloc);
			lputl(t->flag);
			lputl(0);	/* reserved */
			lputl(0);	/* reserved */
			lputl(0);	/* reserved */
		}
	}

	for(i=0; i<nload; i++) {
		l = &load[i];
		lputl(l->type);
		lputl(4*(l->ndata+2));
		for(j=0; j<l->ndata; j++)
			lputl(l->data[j]);
	}

	return 8*4 + loadsize;
}

void
asmbmacho(void)
{
	vlong v, w, e, va;
	MachoHdr *mh;
	MachoSect *msect;
	MachoSeg *ms;
	MachoLoad *ml;

	/* apple MACH */
	va = INITTEXT - HEADR;
	mh = getMachoHdr();
	switch(thechar){
	default:
		diag("unknown mach architecture");
		errorexit();
	case '7':
		mh->cpu = MACHO_CPU_ARM64;
		mh->subcpu = MACHO_SUBCPU_ARM64_ALL;
		break;
	}
	/*
	 * the arm64 XNU kernel kills (SIGKILL, no diagnostic) any main
	 * executable that is not PIE, so MACHO_PIE is not optional
	 * (which is also why the code generator computes addresses
	 * pc-relatively for -H6, see asmout.c case 66)
	 */
	mh->flags = MACHO_NOUNDEFS | MACHO_DYLDLINK | MACHO_TWOLEVEL | MACHO_PIE;

	/* segment for zero page */
	ms = newMachoSeg("__PAGEZERO", 0);
	ms->vsize = va;

	/*
	 * text: maps the header and load commands too, hence
	 * fileoffset 0 and vaddr va (= INITTEXT - HEADR), as is
	 * the convention for Mach-O executables.
	 */
	v = rnd(HEADR+textsize, INITRND);
	ms = newMachoSeg("__TEXT", 1);
	ms->vaddr = va;
	ms->vsize = v;
	ms->fileoffset = 0;
	ms->filesize = v;
	ms->prot1 = 5;	/* r-x; modern XNU refuses w+x segments */
	ms->prot2 = 5;

	msect = newMachoSect(ms, "__text");
	msect->addr = INITTEXT;
	msect->size = textsize;
	msect->off = HEADR;
	msect->align = 2;
	msect->flag = 0x400;	/* flag - some instructions */

	/*
	 * data: INITDAT was rounded up to INITRND by span(), and asmb()
	 * pads the file up to rnd(datsize, INITRND), so that the
	 * vaddr/fileoffset page congruence required by the kernel holds
	 * for __DATA and __LINKEDIT.
	 */
	w = rnd(datsize+bsssize, INITRND);
	ms = newMachoSeg("__DATA", 2);
	ms->vaddr = INITDAT;
	ms->vsize = w;
	ms->fileoffset = v;
	ms->filesize = rnd(datsize, INITRND);
	ms->prot1 = 3;	/* rw- */
	ms->prot2 = 3;

	msect = newMachoSect(ms, "__data");
	msect->addr = INITDAT;
	msect->size = datsize;
	msect->off = v;
	msect->align = 3;

	msect = newMachoSect(ms, "__bss");
	msect->addr = INITDAT+datsize;
	msect->size = bsssize;
	msect->flag = 1;	/* flag - zero fill */

	/*
	 * an empty __LINKEDIT segment at the end of the file.
	 * codesign(1) requires it to exist and extends it with the
	 * code signature that the arm64 kernel insists on.
	 */
	ms = newMachoSeg("__LINKEDIT", 0);
	ms->vaddr = INITDAT + w;
	ms->vsize = 0;
	ms->fileoffset = v + rnd(datsize, INITRND);
	ms->filesize = 0;
	ms->prot1 = 1;	/* r-- */
	ms->prot2 = 1;

	/* minimum OS version, so that tools know this targets macOS */
	ml = newMachoLoad(MACHO_BUILD_VERSION, 4);
	ml->data[0] = MACHO_PLATFORM_MACOS;
	ml->data[1] = 11<<16;	/* minos 11.0.0, first arm64 macOS */
	ml->data[2] = 11<<16;	/* sdk 11.0.0 */
	ml->data[3] = 0;	/* ntools */

	/*
	 * the pre-dyld way to specify the entry point of a static
	 * executable was an LC_UNIXTHREAD command holding the initial
	 * register state (pc = entry) of the main thread. Alas the
	 * arm64 XNU kernel kills static executables at exec() time
	 * (even Apple's own 'ld -static' output!), so we must produce
	 * a dynamic executable instead: dyld as the "interpreter",
	 * at least one dylib, and the LC_MAIN entry point convention.
	 */
	ml = newMachoLoad(MACHO_SYMTAB, 4);	/* all zero: no symbols */
	USED(ml);

	ml = newMachoLoad(MACHO_DYSYMTAB, 18);	/* all zero */
	USED(ml);

	ml = newMachoLoad(MACHO_LOAD_DYLINKER, 6);
	ml->data[0] = 12;	/* offset to string */
	strcpy((char*)&ml->data[1], "/usr/lib/dyld");

	ml = newMachoLoad(MACHO_LOAD_DYLIB,
		4+(strlen("/usr/lib/libSystem.B.dylib")+1+7)/8*2);
	ml->data[0] = 24;	/* offset of string from beginning of load */
	ml->data[1] = 0;	/* time stamp */
	ml->data[2] = 1<<16;	/* version */
	ml->data[3] = 1<<16;	/* compatibility version */
	strcpy((char*)&ml->data[4], "/usr/lib/libSystem.B.dylib");

	/* entry point, called (like a C function) by dyld once done */
	e = entryvalue();
	ml = newMachoLoad(MACHO_MAIN, 4);
	ml->data[0] = e - INITTEXT + HEADR;	/* entryoff: file offset of entry */
	ml->data[1] = (uvlong)(e - INITTEXT + HEADR)>>32;
	ml->data[2] = 0;	/* initial stack size (0 = default) */
	ml->data[3] = 0;

	v = machowrite();
	if(v > HEADR)
		diag("HEADR too small: %lld > %d", v, HEADR);
}
