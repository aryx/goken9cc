#include	"l.h"

int32	OFFSET;

#define PADDR(a)	((a) & ~0xfffffffff0000000ull)

vlong
entryvalue(void)
{
	char *a;
	Sym *s;

	a = INITENTRY;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return INITTEXT;
	switch(s->type) {
	case STEXT:
	case SLEAF:
		break;
	case SDATA:
		if(dlm)
			return s->value+INITDAT;
	default:
		diag("entry not text: %s", s->name);
	}
	return s->value;
}

void
asmb(void)
{
	Prog *p;
	int32 magic, t, etext;
	vlong vl;
	Optab *o;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);
	OFFSET = HEADR;
	seek(cout, OFFSET, SEEK__START);
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + PCSZ;
		}
		if(p->as == ADWORD && (pc & 7) != 0) {
			lputl(0);
			pc += 4;
		}
		if(p->pc != pc) {
			diag("phase error %llux sb %llux",
				p->pc, pc);
			if(!debug['a'])
				prasm(curp);
			pc = p->pc;
		}
		curp = p;
		o = oplook(p);	/* could probably avoid this call */
		asmout(p, o);
		pc += o->size;
	}

	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();

	/* output strings in text segment */
	etext = INITTEXT + textsize;
	for(t = pc; t < etext; t += sizeof(buf)-100) {
		if(etext-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 1);
		else
			datblk(t, etext-t, 1);
	}

	curtext = P;
	switch(HEADTYPE) {
	case 0:
	case 2:
		OFFSET = HEADR+textsize;
		seek(cout, OFFSET, SEEK__START);
		break;
	case 6:	/* apple MACH (Mach-O): __DATA starts at a page boundary too */
	case 7:
        //NEW: ELF Linux constrains that virtual
        // address modulo page must match file offset modulo
        // page, so simpler to start data at a page boundary
        //coupling: must match the code generating the ELF
        // section and ELF program header in elf.c
		OFFSET = rnd(HEADR+textsize, INITRND);
		seek(cout, OFFSET, SEEK__START);
		break;
	}
	if(dlm){
		char buf[8];

		write(cout, buf, INITDAT-textsize);
		textsize = INITDAT;
	}
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 0);
		else
			datblk(t, datsize-t, 0);
	}

	symsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		case 0:
		case 6:	/* Mach-O: emit its own (empty) symtab in asmbmacho, skip plan9 syms */
			debug['s'] = true;
			break;
		case 2:
			OFFSET = HEADR+textsize+datsize;
			seek(cout, OFFSET, SEEK__START);
			break;
        //TODO: no symbol table for ELF Linux?
		case 7:
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f pc\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		if(dlm)
			asmdyn();
		cflush();
	}
	else if(dlm){
		seek(cout, HEADR+textsize+datsize, 0);
		asmdyn();
		cflush();
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f header\n", cputime());
	Bflush(&bso);

	OFFSET = 0;
	seek(cout, OFFSET, SEEK__START);
	switch(HEADTYPE) {
	case 0:	/* no header */
		break;
	case 2:	/* plan 9 */
		magic = 4*28*28+7;
		magic |= 0x00008000;		/* fat header */
		if(dlm)
			magic |= 0x80000000;	/* dlm */
		lput(magic);			/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		vl = entryvalue();
		lput(PADDR(vl));		/* va of entry */
		lput(0L);
		lput(lcsize);
		llput(vl);			/* va of entry */
		break;
	case 7:	/* elf */
		debug['S'] = true;			/* symbol table */
		elf64(ARM64, ELFDATA2LSB, 0, nil);
		break;
	case 6:	/* apple MACH (Mach-O) */
		asmbmacho();
		break;
	}
	cflush();
}

/*
 * Mach-O output for macOS arm64 (modern format).
 *
 * The entry point is an LC_MAIN file offset (not the old LC_UNIXTHREAD
 * register state); the binary is flagged MH_DYLDLINK|MH_TWOLEVEL, links
 * /usr/lib/libSystem.B.dylib, and carries LC_DYLD_INFO_ONLY, LC_BUILD_VERSION
 * and LC_UUID. Because Apple Silicon's kernel (AMFI) SIGKILLs unsigned
 * binaries, we also emit an ad-hoc LC_CODE_SIGNATURE: an embedded SuperBlob
 * with a CodeDirectory whose SHA-256 page hashes cover the whole file image
 * up to the signature. Computing those hashes needs the final bytes on disk,
 * so signing is a second pass that reopens the output and reads it back.
 *
 * Note: the image stays in the low 4GB (small __PAGEZERO, non-PIE) because the
 * classic linker's INITTEXT/INITDAT are int32; a real 0x100000000 __PAGEZERO
 * would require widening those to 64-bit. The signature/load commands - the
 * substantive "modern" parts - do not depend on the load address.
 *
 * File image:
 *   [0, HEADR)            Mach-O header + load commands
 *   [HEADR, HEADR+tsize)  text, mapped at INITTEXT
 *   [v, v+datsize)        data, mapped at INITDAT   (v = rnd(HEADR+tsize,RND))
 *   [linkoff, sigoff)     __LINKEDIT string table (" \0") + pad
 *   [sigoff, sigoff+slen) embedded code signature  (= codeLimit ..)
 */

enum {
	MACHO_CPU_ARM64		= (1<<24)|12,	/* CPU_TYPE_ARM | CPU_ARCH_ABI64 */
	MACHO_SUBCPU_ARM64ALL	= 0,
	CDHASH_PAGE		= 4096,		/* code-signing page size */
};

/*
 * Dynamic imports declared via "7l -I got:remote:lib": the program defines a
 * GOT pointer slot `got` in __DATA and calls through it; we emit an
 * LC_DYLD_INFO non-lazy bind so dyld resolves `remote` from `lib` (libSystem,
 * ordinal 1) into that slot. Mirrors 6l's -I. (See src/cmd/ld/macho.c.)
 */
static struct {
	char	*got;
	char	*remote;
	char	*lib;
} dynimp[16];
static int	ndynimp;
static uchar	machobind[1024];
static int	nmachobind;

void
adddynimp(char *spec)
{
	char *s, *p, *q;

	if(ndynimp >= nelem(dynimp))
		sysfatal("too many -I imports");
	s = strdup(spec);
	p = utfrune(s, ':');
	q = p ? utfrune(p+1, ':') : nil;
	if(p == nil || q == nil)
		sysfatal("bad -I import (want got:remote:lib): %s", spec);
	*p = '\0';
	*q = '\0';
	dynimp[ndynimp].got = s;
	dynimp[ndynimp].remote = p+1;
	dynimp[ndynimp].lib = q+1;
	ndynimp++;
}

static void
binduleb(vlong val)
{
	uchar b;

	do {
		b = val & 0x7f;
		val >>= 7;
		if(val)
			b |= 0x80;
		machobind[nmachobind++] = b;
	} while(val);
}

static void
machoseg(char *name, vlong vaddr, vlong vsize, vlong fileoff, vlong filesize,
	uint32 maxprot, uint32 initprot, int nsect)
{
	lputl(0x19);			/* LC_SEGMENT_64 */
	lputl(72 + 80*nsect);		/* cmdsize */
	strnput(name, 16);
	llputl(vaddr);
	llputl(vsize);
	llputl(fileoff);
	llputl(filesize);
	lputl(maxprot);
	lputl(initprot);
	lputl(nsect);
	lputl(0);			/* flags */
}

static void
machosect(char *sect, char *seg, vlong addr, vlong size, uint32 off,
	uint32 align, uint32 flag)
{
	strnput(sect, 16);
	strnput(seg, 16);
	llputl(addr);
	llputl(size);
	lputl(off);
	lputl(align);
	lputl(0);			/* reloff */
	lputl(0);			/* nreloc */
	lputl(flag);
	lputl(0);			/* reserved1 */
	lputl(0);			/* reserved2 */
	lputl(0);			/* reserved3 */
}

void
asmbmacho(void)
{
	vlong va, v, linkoff, bindoff, sigoff, codeLimit, linkva, llfsize, entryoff, p;
	uint32 identlen, nslot, hashOffset, cdLength, superLength;
	uint32 i, n;
	int sigfd;
	uchar *digests, *buf;
	char *ident, *s;

	/* code-signing identifier: the output file's basename, as codesign does */
	ident = outfile;
	for(s = outfile; *s; s++)
		if(*s == '/')
			ident = s+1;
	if(*ident == '\0')
		ident = "a.out";
	va = INITTEXT - HEADR;			/* address the header maps to */
	v = rnd(HEADR+textsize, INITRND);	/* text seg file/vm size */
	linkoff = v + datsize;			/* file offset of __LINKEDIT */

	/* dyld non-lazy bind opcodes for -I imports, right after the string table */
	bindoff = linkoff + 2;
	nmachobind = 0;
	if(ndynimp > 0) {
		Sym *isym;
		char *cp;

		for(i=0; i<(uint32)ndynimp; i++) {
			isym = lookup(dynimp[i].got, 0);
			machobind[nmachobind++] = 0x10 | 1;	/* SET_DYLIB_ORDINAL_IMM, libSystem */
			machobind[nmachobind++] = 0x40;		/* SET_SYMBOL_TRAILING_FLAGS_IMM, 0 */
			for(cp = dynimp[i].remote; *cp; cp++)
				machobind[nmachobind++] = *cp;
			machobind[nmachobind++] = 0;
			machobind[nmachobind++] = 0x50 | 1;	/* SET_TYPE_IMM, BIND_TYPE_POINTER */
			machobind[nmachobind++] = 0x70 | 2;	/* SET_SEGMENT_AND_OFFSET_ULEB, __DATA */
			binduleb(isym->value);			/* slot offset within __DATA */
			machobind[nmachobind++] = 0x90;		/* DO_BIND */
		}
		machobind[nmachobind++] = 0x00;			/* DONE */
	}

	sigoff = rnd(bindoff + nmachobind, 16);	/* signature follows strtab + binds */
	codeLimit = sigoff;			/* everything before the signature is hashed */
	linkva = rnd(INITDAT+datsize+bsssize, INITRND);
	entryoff = entryvalue() - va;		/* LC_MAIN entry, offset into __TEXT */

	identlen = strlen(ident) + 1;
	nslot = (codeLimit + CDHASH_PAGE-1) / CDHASH_PAGE;
	hashOffset = 88 + identlen;		/* 88 = CodeDirectory v0x20400 header */
	cdLength = hashOffset + nslot*32;	/* nSpecialSlots = 0 */
	superLength = 12 + 8 + cdLength;	/* SuperBlob hdr + 1 index + CodeDirectory */
	llfsize = (sigoff + superLength) - linkoff;	/* __LINKEDIT file size */

	/* __LINKEDIT: string table " \0", then bind opcodes, zero-padded to the signature */
	seek(cout, linkoff, SEEK__START);
	cput(' ');
	cput(0);
	for(i=0; i<(uint32)nmachobind; i++)
		cput(machobind[i]);
	for(p = bindoff + nmachobind; p < sigoff; p++)
		cput(0);
	cflush();

	/* header + load commands at the start of the file */
	seek(cout, 0, SEEK__START);

	lputl(0xfeedfacf);			/* MH_MAGIC_64 */
	lputl(MACHO_CPU_ARM64);
	lputl(MACHO_SUBCPU_ARM64ALL);
	lputl(2);				/* MH_EXECUTE */
	lputl(13);				/* ncmds */
	lputl(856);				/* sizeofcmds */
	lputl(1 | 4 | 0x80 | 0x200000);		/* MH_NOUNDEFS|MH_DYLDLINK|MH_TWOLEVEL|MH_PIE */
	lputl(0);				/* reserved */

	machoseg("__PAGEZERO", 0, va, 0, 0, 0, 0, 0);
	machoseg("__TEXT", va, v, 0, v, 7, 5, 1);
	machosect("__text", "__TEXT", INITTEXT, textsize, HEADR, 2, 0x80000400);
	machoseg("__DATA", INITDAT, datsize+bsssize, v, datsize, 7, 3, 2);
	machosect("__data", "__DATA", INITDAT, datsize, v, 0, 0);
	machosect("__bss", "__DATA", INITDAT+datsize, bsssize, 0, 0, 1);
	machoseg("__LINKEDIT", linkva, rnd(llfsize, INITRND), linkoff, llfsize, 7, 1, 0);

	/* LC_DYLD_INFO_ONLY - non-lazy binds for -I imports (rest empty) */
	lputl(0x80000022); lputl(48);
	lputl(0); lputl(0);			/* rebase off/size */
	lputl(nmachobind ? bindoff : 0); lputl(nmachobind);	/* bind off/size */
	lputl(0); lputl(0);			/* weak bind off/size */
	lputl(0); lputl(0);			/* lazy bind off/size */
	lputl(0); lputl(0);			/* export off/size */

	/* LC_SYMTAB - empty symbol table, 2-byte string table */
	lputl(2); lputl(24);
	lputl(linkoff);	lputl(0); lputl(linkoff); lputl(2);

	/* LC_DYSYMTAB - empty */
	lputl(11); lputl(80);
	for(i=0; i<18; i++) lputl(0);

	/* LC_LOAD_DYLINKER */
	lputl(14); lputl(32);
	lputl(12);
	strnput("/usr/lib/dyld", 20);

	/* LC_UUID (deterministic placeholder) */
	lputl(0x1b); lputl(24);
	lputl(0x6b636e6b); lputl(0x36303963); lputl(0x6f72636d); lputl(0x00000073);

	/* LC_BUILD_VERSION (macOS 11.0, first arm64 release) */
	lputl(0x32); lputl(24);
	lputl(1);				/* PLATFORM_MACOS */
	lputl(0x000b0000);			/* minos 11.0.0 */
	lputl(0x000b0000);			/* sdk   11.0.0 */
	lputl(0);				/* ntools */

	/* LC_MAIN */
	lputl(0x80000028); lputl(24);
	llputl(entryoff);			/* entryoff */
	llputl(0);				/* stacksize */

	/* LC_LOAD_DYLIB libSystem */
	lputl(12); lputl(56);
	lputl(24);				/* name offset */
	lputl(2);				/* timestamp */
	lputl(0x051f0000);			/* current_version */
	lputl(0x00010000);			/* compatibility_version */
	strnput("/usr/lib/libSystem.B.dylib", 32);

	/* LC_CODE_SIGNATURE - points just past codeLimit */
	lputl(0x1d); lputl(16);
	lputl(codeLimit);			/* dataoff */
	lputl(superLength);			/* datasize */

	cflush();

	/*
	 * Second pass: reopen the now-written file, SHA-256 each page of
	 * [0, codeLimit), and emit the ad-hoc embedded signature at sigoff.
	 * Code-signing structures are big-endian (lput/llput).
	 */
	sigfd = open(outfile, OREAD);
	if(sigfd < 0) {
		diag("cannot reopen %s for code signing", outfile);
		return;
	}
	digests = malloc(nslot*32);
	buf = malloc(CDHASH_PAGE);
	for(i = 0; i < nslot; i++) {
		n = codeLimit - (vlong)i*CDHASH_PAGE;
		if(n > CDHASH_PAGE)
			n = CDHASH_PAGE;
		seek(sigfd, (vlong)i*CDHASH_PAGE, SEEK__START);
		readn(sigfd, buf, n);
		sha256(buf, n, digests + i*32);
	}
	close(sigfd);

	seek(cout, sigoff, SEEK__START);
	/* CS_SuperBlob */
	lput(0xfade0cc0);			/* CSMAGIC_EMBEDDED_SIGNATURE */
	lput(superLength);
	lput(1);				/* blob count */
	lput(0);				/* CSSLOT_CODEDIRECTORY */
	lput(20);				/* offset of CodeDirectory */
	/* CS_CodeDirectory (version 0x20400) */
	lput(0xfade0c02);			/* CSMAGIC_CODEDIRECTORY */
	lput(cdLength);
	lput(0x00020400);			/* version */
	lput(0x00000002);			/* flags = CS_ADHOC */
	lput(hashOffset);
	lput(88);				/* identOffset */
	lput(0);				/* nSpecialSlots */
	lput(nslot);				/* nCodeSlots */
	lput(codeLimit);
	cput(32);				/* hashSize (SHA-256) */
	cput(2);				/* hashType = CS_HASHTYPE_SHA256 */
	cput(0);				/* platform */
	cput(12);				/* pageSize = log2(4096) */
	lput(0);				/* spare2 */
	lput(0);				/* scatterOffset */
	lput(0);				/* teamOffset */
	lput(0);				/* spare3 */
	llput(0);				/* codeLimit64 */
	llput(0);				/* execSegBase  (file offset of __TEXT) */
	llput(v);				/* execSegLimit (__TEXT filesize) */
	llput(1);				/* execSegFlags = CS_EXECSEG_MAIN_BINARY */
	for(i = 0; ident[i]; i++)		/* identifier */
		cput(ident[i]);
	cput(0);
	for(i = 0; i < nslot*32; i++)		/* code page hashes */
		cput(digests[i]);
	cflush();
}

void
wput(int32 l)
{

	cbp[0] = l>>8;
	cbp[1] = l;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
wputl(int32 l)
{

	cbp[0] = l;
	cbp[1] = l>>8;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
lput(int32 l)
{

	cbp[0] = l>>24;
	cbp[1] = l>>16;
	cbp[2] = l>>8;
	cbp[3] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
lputl(int32 l)
{

	cbp[3] = l>>24;
	cbp[2] = l>>16;
	cbp[1] = l>>8;
	cbp[0] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
llput(vlong v)
{
	lput(v>>32);
	lput(v);
}

void
llputl(vlong v)
{
	lputl(v);
	lputl(v>>32);
}

void
asmsym(void)
{
	Prog *p;
	Auto *a;
	Sym *s;
	int h;

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SCONST:
				putsymb(s->name, 'D', s->value, s->version);
				continue;

			case SDATA:
				putsymb(s->name, 'D', s->value+INITDAT, s->version);
				continue;

			case SBSS:
				putsymb(s->name, 'B', s->value+INITDAT, s->version);
				continue;

			case SSTRING:
				putsymb(s->name, 'T', s->value, s->version);
				continue;

			case SFILE:
				putsymb(s->name, 'f', s->value, s->version);
				continue;
			}

	for(p=textp; p!=P; p=p->cond) {
		s = p->from.sym;
		if(s->type != STEXT && s->type != SLEAF)
			continue;

		/* filenames first */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_FILE)
				putsymb(a->asym->name, 'z', a->aoffset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->asym->name, 'Z', a->aoffset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+PCSZ, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->asym->name, 'a', -a->aoffset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->asym->name, 'p', a->aoffset, 0);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lud\n", symsize);
	Bflush(&bso);
}

void
putsymb(char *s, int t, vlong v, int ver)
{
	int i, f, l;

	if(t == 'f')
		s++;
	l = 4;
	switch(HEADTYPE){
	default:
		break;
	case 2:
		lput(v>>32);
		l = 8;
		break;
	}
	lput(v);
	if(ver)
		t += 'a' - 'A';
	cput(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		cput(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2) {
			cput(s[i]);
			cput(s[i+1]);
		}
		cput(0);
		cput(0);
		i++;
	}
	else {
		for(i=0; s[i]; i++)
			cput(s[i]);
		cput(0);
	}
	symsize += l + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8llux ", t, v);
			for(i=1; s[i] != 0 || s[i+1] != 0; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8llux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8llux %s\n", t, v, s);
	}
}

#define	MINLC	4
void
asmlc(void)
{
	int32 oldpc, oldlc;
	Prog *p;
	int32 v, s;

	oldpc = INITTEXT;
	oldlc = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->line == oldlc || p->as == ATEXT || p->as == ANOP) {
			if(p->as == ATEXT)
				curtext = p;
			if(debug['V'])
				Bprint(&bso, "%6llux %P\n",
					p->pc, p);
			continue;
		}
		if(debug['V'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			cput(s+128);	/* 129-255 +pc */
			if(debug['V'])
				Bprint(&bso, " pc+%ld*%d(%ld)", s, MINLC, s+128);
			v -= s;
			lcsize++;
		}
		s = p->line - oldlc;
		oldlc = p->line;
		oldpc = p->pc + MINLC;
		if(s > 64 || s < -64) {
			cput(0);	/* 0 vv +lc */
			cput(s>>24);
			cput(s>>16);
			cput(s>>8);
			cput(s);
			if(debug['V']) {
				if(s > 0)
					Bprint(&bso, " lc+%ld(%d,%ld)\n",
						s, 0, s);
				else
					Bprint(&bso, " lc%ld(%d,%ld)\n",
						s, 0, s);
				Bprint(&bso, "%6llux %P\n",
					p->pc, p);
			}
			lcsize += 5;
			continue;
		}
		if(s > 0) {
			cput(0+s);	/* 1-64 +lc */
			if(debug['V']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6llux %P\n",
					p->pc, p);
			}
		} else {
			cput(64-s);	/* 65-128 -lc */
			if(debug['V']) {
				Bprint(&bso, " lc%ld(%ld)\n", s, 64-s);
				Bprint(&bso, "%6llux %P\n",
					p->pc, p);
			}
		}
		lcsize++;
	}
	while(lcsize & 1) {
		s = 129;
		cput(s);
		lcsize++;
	}
	if(debug['v'] || debug['V'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	Bflush(&bso);
}

void
datblk(int32 s, int32 n, int str)
{
	Sym *v;
	Prog *p;
	char *cast;
	int32 a, l, fl, j;
	vlong d;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		if(str != (p->from.sym->type == SSTRING))
			continue;
		curp = p;
		a = p->from.sym->value + p->from.offset;
		l = a - s;
		c = p->reg;
		i = 0;
		if(l < 0) {
			if(l+c <= 0)
				continue;
			while(l < 0) {
				l++;
				i++;
			}
		}
		if(l >= n)
			continue;
		if(p->as != AINIT && p->as != ADYNT) {
			for(j=l+(c-i)-1; j>=l; j--)
				if(buf.dbuf[j]) {
					print("%P\n", p);
					diag("multiple initialization");
					break;
				}
		}
		switch(p->to.type) {
		default:
			diag("unknown mode in initialization%P", p);
			break;

		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi4[i]];
					l++;
				}
				break;
			case 8:
				cast = (char*)p->to.ieee;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
					l++;
				}
				break;
			}
			break;

		case D_SCONST:
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_CONST:
			d = p->to.offset;
			v = p->to.sym;
			if(v) {
				switch(v->type) {
				case SUNDEF:
					ckoff(v, d);
				case STEXT:
				case SLEAF:
				case SSTRING:
					d += p->to.sym->value;
					break;
				case SDATA:
				case SBSS:
					d += p->to.sym->value + INITDAT;
				}
				if(dlm)
					dynreloc(v, a+INITDAT, 1);
			}
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d%P", c, i, curp);
				break;
			case 1:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi4[i]];
					l++;
				}
				break;
			case 8:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi8[i]];
					l++;
				}
				break;
			}
			break;
		}
	}
	write(cout, buf.dbuf, n);
}

static Ieee chipfloats[] = {
	{0x00000000, 0x00000000}, /* 0 */
	{0x00000000, 0x3ff00000}, /* 1 */
	{0x00000000, 0x40000000}, /* 2 */
	{0x00000000, 0x40080000}, /* 3 */
	{0x00000000, 0x40100000}, /* 4 */
	{0x00000000, 0x40140000}, /* 5 */
	{0x00000000, 0x3fe00000}, /* .5 */
	{0x00000000, 0x40240000}, /* 10 */
};

int
chipfloat(Ieee *e)
{
	Ieee *p;
	int n;

	for(n = sizeof(chipfloats)/sizeof(chipfloats[0]); --n >= 0;){
		p = &chipfloats[n];
		if(p->l == e->l && p->h == e->h && 0)
			return n;		/* TO DO: return imm8 encoding */
	}
	return -1;
}
