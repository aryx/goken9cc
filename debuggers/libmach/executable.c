#include	<lib9.h>
#include	<bio.h>
#include	"bootexec.h"
#include	"mach.h"

#include	"elf.h"
typedef u32int uint32;
typedef int int32;
#include	"macho.h"

/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of longs
 */

typedef struct {
	union{
		Exec exec;			/* in a.out.h */
		Ehdr ehdr;			/* in elf.h */
		Machhdr machhdr;	/* macho.h */
	} e;
	long dummy;		/* padding to ensure extra long */
} ExecHdr;

static	int	common(int, Fhdr*, ExecHdr*);
static	int	machdotout(int, Fhdr*, ExecHdr*);
static	int	elfdotout(int, Fhdr*, ExecHdr*);
static	int	armdotout(int, Fhdr*, ExecHdr*);
static	void	setsym(Fhdr*, vlong, int32, vlong, int32, vlong, int32);
static	void	setdata(Fhdr*, long, long, long, long);
static	void	settext(Fhdr*, long, long, long, long);
static	void	hswal(long*, int, long(*)(long));
static	long	_round(long, long);

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	long	magic;			/* big-endian magic number of file */
	char	*name;			/* executable identifier */
	int	type;			/* Internal code */
	Mach	*mach;			/* Per-machine data */
	ulong	hsize;			/* header size */
	long	(*swal)(long);		/* beswal or leswal */
	int	(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

extern	Mach	mi386;
extern	Mach	marm;

ExecTable exectab[] =
{
	{ I_MAGIC,			/* I386 8.out & boot image */
		"386 plan 9 executable",
		FI386,
		&mi386,
		sizeof(Exec),
		beswal,
		common },
	{ E_MAGIC,			/* Arm 5.out */
		"Arm plan 9 executable",
		FARM,
		&marm,
		sizeof(Exec),
		beswal,
		common },
//	{ ELF_MAG,			/* any elf32 or elf64 */
//		"elf executable",
//		nil,
//		FNONE,
//		0,
//		&mi386,
//		sizeof(Ehdr64),
//		nil,
//		elfdotout },
	{ MACH32_MAG,			/* 32-bit MACH (apple mac) */
		"mach executable",
		FI386,
		&mi386,
		sizeof(Machhdr),
		leswal, // used to be nil
		machdotout },
	{ 0 },
};

Mach	*mach = &mi386;			/* Global current machine table */

int
crackhdr(int fd, Fhdr *fp)
{
	ExecTable *mp;
	ExecHdr d;
	int nb, magic, ret;

	fp->type = FNONE;
	nb = read(fd, (char *)&d.e, sizeof(d.e));
	if (nb <= 0)
		return 0;

	ret = 0;
	fp->magic = magic = beswal(d.e.exec.magic);		/* big-endian */
	for (mp = exectab; mp->magic; mp++) {
		if (mp->magic == magic && nb >= mp->hsize) {
			//if(mp->magic == V_MAGIC)
			//	mp = couldbe4k(mp);

			hswal((long *) &d, sizeof(d.e)/sizeof(long), mp->swal);
			fp->type = mp->type;
			fp->name = mp->name;
			fp->hdrsz = mp->hsize;		/* zero on bootables */
			mach = mp->mach;
			ret  = mp->hparse(fd, fp, &d);
			seek(fd, mp->hsize, 0);		/* seek to end of header */
			break;
		}
	}
	if(mp->magic == 0)
		werrstr("unknown header type");
	return ret;
}
/*
 * Convert header to canonical form
 */
static void
hswal(long *lp, int n, long (*swap) (long))
{
	while (n--) {
		*lp = (*swap) (*lp);
		lp++;
	}
}
/*
 *	Crack a normal a.out-type header
 */
static int
adotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	long pgsize;

	USED(fd);
	pgsize = mach->pgsize;
	settext(fp, hp->e.exec.entry, pgsize+sizeof(Exec),
			hp->e.exec.text, sizeof(Exec));
	setdata(fp, _round(pgsize+fp->txtsz+sizeof(Exec), pgsize),
		hp->e.exec.data, fp->txtsz+sizeof(Exec), hp->e.exec.bss);
	setsym(fp, fp->datoff+fp->datsz, hp->e.exec.syms, 0, hp->e.exec.spsz, 0, hp->e.exec.pcsz);
	return 1;
}

/*
 *	68020 2.out and 68020 bootable images
 *	386I 8.out and 386I bootable images
 *
 */
static int
common(int fd, Fhdr *fp, ExecHdr *hp)
{
	long kbase;

	adotout(fd, fp, hp);
	kbase = mach->kbase;
	if ((fp->entry & kbase) == kbase) {		/* Boot image */
		switch(fp->type) {
		case F68020:
			fp->type = F68020B;
			fp->name = "68020 plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			break;
		case FI386:
			fp->type = FI386B;
			fp->txtaddr = sizeof(Exec);
			fp->name = "386 plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			fp->dataddr = fp->txtaddr+fp->txtsz;
			break;
		case FARM:
			fp->txtaddr = kbase+0x8000+sizeof(Exec);
			fp->name = "ARM plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			fp->dataddr = fp->txtaddr+fp->txtsz;
			return 1;
		default:
			break;
		}
		fp->txtaddr |= kbase;
		fp->entry |= kbase;
		fp->dataddr |= kbase;
	}
	else if (fp->type == FARM && (fp->entry == 0x8020 || fp->entry == 0x8080)) {
		fp->txtaddr = fp->entry;
		fp->name = "ARM Inferno boot image";
		fp->hdrsz = 0;		/* header stripped */
		fp->dataddr = fp->txtaddr+fp->txtsz;
	}
	else if (fp->type == FPOWER && fp->entry == 0x3020) {
		fp->txtaddr = fp->entry;
		fp->name = "Power Inferno boot image";
		fp->hdrsz = 0;		/* header stripped */
		fp->dataddr = fp->txtaddr+fp->txtsz;
	}
	return 1;
}


static Shdr*
elfsectbyname(int fd, Ehdr *hp, Shdr *sp, char *name)
{
	int i, offset, n;
	char s[64];

	offset = sp[hp->shstrndx].offset;
	for(i = 1; i < hp->shnum; i++) {
		seek(fd, offset+sp[i].name, 0);
		n = read(fd, s, sizeof(s)-1);
		if(n < 0)
			continue;
		s[n] = 0;
		if(strcmp(s, name) == 0)
			return &sp[i]; 
	}
	return 0;
}
/*
 *	Decode an Irix 5.x ELF header
 */
static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{

	Ehdr *ep;
	Shdr *es, *txt, *init, *s;
	long addr, size, offset, bsize;

	ep = &hp->e.ehdr;
	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;

	if(ep->shnum <= 0) {
		werrstr("no ELF header sections");
		return 0;
	}
	es = malloc(sizeof(Shdr)*ep->shnum);
	if(es == 0)
		return 0;

	seek(fd, ep->shoff, 0);
	if(read(fd, es, sizeof(Shdr)*ep->shnum) < 0){
		free(es);
		return 0;
	}

	txt = elfsectbyname(fd, ep, es, ".text");
	init = elfsectbyname(fd, ep, es, ".init");
	if(txt == 0 || init == 0 || init != txt+1)
		goto bad;
	if(txt->addr+txt->size != init->addr)
		goto bad;
	settext(fp, ep->elfentry, txt->addr, txt->size+init->size, txt->offset);

	addr = 0;
	offset = 0;
	size = 0;
	s = elfsectbyname(fd, ep, es, ".data");
	if(s) {
		addr = s->addr;
		size = s->size;
		offset = s->offset;
	}

	s = elfsectbyname(fd, ep, es, ".rodata");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		size += s->size;
	}

	s = elfsectbyname(fd, ep, es, ".got");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		size += s->size;
	}

	bsize = 0;
	s = elfsectbyname(fd, ep, es, ".bss");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		bsize = s->size;
	}

	if(addr == 0)
		goto bad;

	setdata(fp, addr, size, offset, bsize);
	fp->name = "IRIX Elf a.out executable";
	free(es);
	return 1;
bad:
	free(es);
	werrstr("ELF sections scrambled");
	return 0;
}


static int
machdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	uvlong (*swav)(uvlong);
	uint32 (*swal)(uint32);
	Machhdr *mp;
	MachCmd **cmd;
	MachSymSeg *symtab;
	MachSymSeg *pclntab;
	MachSeg64 *seg;
	MachSect64 *sect;
	MachSeg32 *seg32;
	MachSect32 *sect32;
	uvlong textsize, datasize, bsssize;
	uchar *cmdbuf;
	uchar *cmdp;
	int i, j, hdrsize;
	uint32 textva, textoff, datava, dataoff, symoff, symsize, pclnoff, pclnsize;

	mp = &hp->e.machhdr;
	if (leswal(mp->filetype) != MACH_EXECUTABLE_TYPE) {
		werrstr("bad MACH executable type %#ux", leswal(mp->filetype));
		return 0;
	}

	swal = leswal;
	swav = leswav;

	mp->magic = swal(mp->magic);
	mp->cputype = swal(mp->cputype);
	mp->cpusubtype = swal(mp->cpusubtype);
	mp->filetype = swal(mp->filetype);
	mp->ncmds = swal(mp->ncmds);
	mp->sizeofcmds = swal(mp->sizeofcmds);
	mp->flags = swal(mp->flags);
	mp->reserved = swal(mp->reserved);

	switch(mp->magic) {
	case 0xFEEDFACE:	// 32-bit mach
		if (mp->cputype != MACH_CPU_TYPE_X86) {
			werrstr("bad MACH cpu type - not 386");
			return 0;
		}
		if (mp->cpusubtype != MACH_CPU_SUBTYPE_X86) {
			werrstr("bad MACH cpu subtype - not 386");
			return 0;
		}
		if (mp->filetype != MACH_EXECUTABLE_TYPE) {
			werrstr("bad MACH executable type");
			return 0;
		}
		mach = &mi386;
		fp->type = FI386;
		hdrsize = 28;
		break;

	case 0xFEEDFACF:	// 64-bit mach
		werrstr("64 bit mach NOT SUPPORTED");
		return 0;
		//if (mp->cputype != MACH_CPU_TYPE_X86_64) {
		//	werrstr("bad MACH cpu type - not amd64");
		//	return 0;
		//}
        //
		//if (mp->cpusubtype != MACH_CPU_SUBTYPE_X86 && mp->cpusubtype != MACH_CPU_SUBTYPE_X86_64) {
		//	werrstr("bad MACH cpu subtype - not amd64");
		//	return 0;
		//}
		//mach = &mamd64;
		//fp->type = FAMD64;
		//hdrsize = 32;
		//break;

	default:
		werrstr("not mach %#ux", mp->magic);
		return 0;
	}

	cmdbuf = malloc(mp->sizeofcmds);
	if(!cmdbuf) {
		werrstr("out of memory");
		return 0;
	}
	seek(fd, hdrsize, 0);
	if(read(fd, cmdbuf, mp->sizeofcmds) != mp->sizeofcmds) {
		free(cmdbuf);
		return 0;
	}
	cmd = malloc(mp->ncmds * sizeof(MachCmd*));
	if(!cmd) {
		free(cmdbuf);
		werrstr("out of memory");
		return 0;
	}
	cmdp = cmdbuf;
	textva = 0;
	textoff = 0;
	dataoff = 0;
	datava = 0;
	symtab = 0;
	pclntab = 0;
	textsize = 0;
	datasize = 0;
	bsssize = 0;
	symoff = 0;
	symsize = 0;
	pclnoff = 0;
	pclnsize = 0;
	for (i = 0; i < mp->ncmds; i++) {
		MachCmd *c;

		cmd[i] = (MachCmd*)cmdp;
		c = cmd[i];
		c->type = swal(c->type);
		c->size = swal(c->size);
		switch(c->type) {
		case MACH_SEGMENT_32:
			if(mp->magic != 0xFEEDFACE) {
				werrstr("segment 32 in mach 64");
				goto bad;
			}
			seg32 = (MachSeg32*)c;
			seg32->vmaddr = swav(seg32->vmaddr);
			seg32->vmsize = swav(seg32->vmsize);
			seg32->fileoff = swav(seg32->fileoff);
			seg32->filesize = swav(seg32->filesize);
			seg32->maxprot = swal(seg32->maxprot);
			seg32->initprot = swal(seg32->initprot);
			seg32->nsects = swal(seg32->nsects);
			seg32->flags = swal(seg32->flags);
			if (strcmp(seg32->segname, "__TEXT") == 0) {
				textva = seg32->vmaddr;
				textoff = seg32->fileoff;
				textsize = seg32->vmsize;
				sect32 = (MachSect32*)(cmdp + sizeof(MachSeg32));
				for(j = 0; j < seg32->nsects; j++, sect32++) {
					if (strcmp(sect32->sectname, "__gosymtab") == 0) {
						symoff = swal(sect32->offset);
						symsize = swal(sect32->size);
					}
					if (strcmp(sect32->sectname, "__gopclntab") == 0) {
						pclnoff = swal(sect32->offset);
						pclnsize = swal(sect32->size);
					}
				}
			}
			if (strcmp(seg32->segname, "__DATA") == 0) {
				datava = seg32->vmaddr;
				dataoff = seg32->fileoff;
				datasize = seg32->filesize;
				bsssize = seg32->vmsize - seg32->filesize;
			}
			break;

		case MACH_SEGMENT_64:
			werrstr("MACH SEGMENT 64 NOT SUPPORTED");
				goto bad;
			//if(mp->magic != 0xFEEDFACF) {
			//	werrstr("segment 32 in mach 64");
			//	goto bad;
			//}
			//seg = (MachSeg64*)c;
			//seg->vmaddr = swav(seg->vmaddr);
			//seg->vmsize = swav(seg->vmsize);
			//seg->fileoff = swav(seg->fileoff);
			//seg->filesize = swav(seg->filesize);
			//seg->maxprot = swal(seg->maxprot);
			//seg->initprot = swal(seg->initprot);
			//seg->nsects = swal(seg->nsects);
			//seg->flags = swal(seg->flags);
			//if (strcmp(seg->segname, "__TEXT") == 0) {
			//	textva = seg->vmaddr;
			//	textoff = seg->fileoff;
			//	textsize = seg->vmsize;
			//	sect = (MachSect64*)(cmdp + sizeof(MachSeg64));
			//	for(j = 0; j < seg->nsects; j++, sect++) {
			//		if (strcmp(sect->sectname, "__gosymtab") == 0) {
			//			symoff = swal(sect->offset);
			//			symsize = swal(sect->size);
			//		}
			//		if (strcmp(sect->sectname, "__gopclntab") == 0) {
			//			pclnoff = swal(sect->offset);
			//			pclnsize = swal(sect->size);
			//		}
			//	}
			//}
			//if (strcmp(seg->segname, "__DATA") == 0) {
			//	datava = seg->vmaddr;
			//	dataoff = seg->fileoff;
			//	datasize = seg->filesize;
			//	bsssize = seg->vmsize - seg->filesize;
			//}
			break;
		case MACH_UNIXTHREAD:
			break;
		case MACH_SYMSEG:
			if (symtab == 0) {
				symtab = (MachSymSeg*)c;
				symoff = swal(symtab->fileoff);
				symsize = swal(symtab->filesize);
			} else if (pclntab == 0) {
				pclntab = (MachSymSeg*)c;
				pclnoff = swal(pclntab->fileoff);
				pclnsize = swal(pclntab->filesize);
			}
			break;
		}
		cmdp += c->size;
	}
	if (textva == 0 || datava == 0) {
		free(cmd);
		free(cmdbuf);
		return 0;
	}
	/* compute entry by taking address after header - weird - BUG? */
	settext(fp, textva+sizeof(Machhdr) + mp->sizeofcmds, textva, textsize, textoff);
	setdata(fp, datava, datasize, dataoff, bsssize);
	if(symoff > 0)
		setsym(fp, symoff, symsize, 0, 0, pclnoff, pclnsize);
	free(cmd);
	free(cmdbuf);
	return 1;
bad:
	free(cmd);
	free(cmdbuf);
	return 0;
}


/*
 * (Free|Net)BSD ARM header.
 */
static int
armdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	long kbase;

	USED(fd);
	settext(fp, hp->e.exec.entry, sizeof(Exec), hp->e.exec.text, sizeof(Exec));
	setdata(fp, fp->txtsz, hp->e.exec.data, fp->txtsz, hp->e.exec.bss);
	setsym(fp, fp->datoff+fp->datsz, hp->e.exec.syms, 0, hp->e.exec.spsz, 0, hp->e.exec.pcsz);

	kbase = 0xF0000000;
	if ((fp->entry & kbase) == kbase) {		/* Boot image */
		fp->txtaddr = kbase+sizeof(Exec);
		fp->name = "ARM *BSD boot image";
		fp->hdrsz = 0;		/* header stripped */
		fp->dataddr = kbase+fp->txtsz;
	}
	return 1;
}

static void
settext(Fhdr *fp, long e, long a, long s, long off)
{
	fp->txtaddr = a;
	fp->entry = e;
	fp->txtsz = s;
	fp->txtoff = off;
}
static void
setdata(Fhdr *fp, long a, long s, long off, long bss)
{
	fp->dataddr = a;
	fp->datsz = s;
	fp->datoff = off;
	fp->bsssz = bss;
}

static void
setsym(Fhdr *fp, vlong symoff, int32 symsz, vlong sppcoff, int32 sppcsz, vlong lnpcoff, int32 lnpcsz)
{
	fp->symoff = symoff;
	fp->symsz = symsz;
	
	if(sppcoff == 0)
		sppcoff = symoff+symsz;
	fp->sppcoff = symoff;
	fp->sppcsz = sppcsz;

	if(lnpcoff == 0)
		lnpcoff = sppcoff + sppcsz;
	fp->lnpcoff = lnpcoff;
	fp->lnpcsz = lnpcsz;
}


static long
_round(long a, long b)
{
	long w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}
