
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


// claude: struct i386exec used to live here -- every one of its own
// fields was already commented out "UNUSED" by the original author,
// leaving an empty `struct i386exec { };` body that this compiler
// can't parse ("syntax error, last name: i386exec": empty structs
// aren't legal in this C89-ish dialect) and that's referenced nowhere
// else in libmach (grepped, not assumed) -- deleted rather than kept
// as a stub, per this project's "don't add backwards-compat hacks for
// things confirmed unused" convention.
