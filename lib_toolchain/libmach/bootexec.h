
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
