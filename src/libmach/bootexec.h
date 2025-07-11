
struct coffsect
{
	char	name[8];
	uint32	phys;
	uint32	virt;
	uint32	size;
	uint32	fptr;
	uint32	fptrreloc;
	uint32	fptrlineno;
	uint32	nrelocnlineno;
	uint32	flags;
};

/*
 * proprietary exec headers, needed to bootstrap various machines
 */
struct mipsexec
{
	short	mmagic;		/* (0x160) mips magic number */
	short	nscns;		/* (unused) number of sections */
	int32	timdat;		/* (unused) time & date stamp */
	int32	symptr;		/* offset to symbol table */
	int32	nsyms;		/* size of symbol table */
	short	opthdr;		/* (0x38) sizeof(optional hdr) */
	short	pcszs;		/* flags */
	short	amagic;		/* see above */
	short	vstamp;		/* version stamp */
	int32	tsize;		/* text size in bytes */
	int32	dsize;		/* initialized data */
	int32	bsize;		/* uninitialized data */
	int32	mentry;		/* entry pt.				*/
	int32	text_start;	/* base of text used for this file	*/
	int32	data_start;	/* base of data used for this file	*/
	int32	bss_start;	/* base of bss used for this file	*/
	int32	gprmask;	/* general purpose register mask	*/
union{
	int32	cprmask[4];	/* co-processor register masks		*/
	int32	pcsize;
};
	int32	gp_value;	/* the gp value used for this object    */
};

struct mips4kexec
{
	struct mipsexec	h;
	struct coffsect	itexts;
	struct coffsect idatas;
	struct coffsect ibsss;
};

struct sparcexec
{
	short	sjunk;		/* dynamic bit and version number */
	short	smagic;		/* 0407 */
	uint32	stext;
	uint32	sdata;
	uint32	sbss;
	uint32	ssyms;
	uint32	sentry;
	uint32	strsize;
	uint32	sdrsize;
};

struct nextexec
{
/* UNUSED
	struct	nexthdr{
		uint32	nmagic;
		uint32	ncputype;
		uint32	ncpusubtype;
		uint32	nfiletype;
		uint32	ncmds;
		uint32	nsizeofcmds;
		uint32	nflags;
	};

	struct nextcmd{
		uint32	cmd;
		uint32	cmdsize;
		uchar	segname[16];
		uint32	vmaddr;
		uint32	vmsize;
		uint32	fileoff;
		uint32	filesize;
		uint32	maxprot;
		uint32	initprot;
		uint32	nsects;
		uint32	flags;
	}textc;
	struct nextsect{
		char	sectname[16];
		char	segname[16];
		uint32	addr;
		uint32	size;
		uint32	offset;
		uint32	align;
		uint32	reloff;
		uint32	nreloc;
		uint32	flags;
		uint32	reserved1;
		uint32	reserved2;
	}texts;
	struct nextcmd	datac;
	struct nextsect	datas;
	struct nextsect	bsss;
	struct nextsym{
		uint32	cmd;
		uint32	cmdsize;
		uint32	symoff;
		uint32	nsyms;
		uint32	spoff;
		uint32	pcoff;
	}symc;
*/
};

struct i386exec
{
/* UNUSED
	struct	i386coff{
		uint32	isectmagic;
		uint32	itime;
		uint32	isyms;
		uint32	insyms;
		uint32	iflags;
	};
	struct	i386hdr{
		uint32	imagic;
		uint32	itextsize;
		uint32	idatasize;
		uint32	ibsssize;
		uint32	ientry;
		uint32	itextstart;
		uint32	idatastart;
	};
	struct coffsect	itexts;
	struct coffsect idatas;
	struct coffsect ibsss;
	struct coffsect icomments;
*/
};
