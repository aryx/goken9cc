/*
 *	Architecture-dependent application data
 */

//******************************************************************************
// Prelude
//******************************************************************************
// Library used by debugger/profiler/... for arch-specific things.
// mach in libmach stands for "machine"
//
// Note that the linker does not rely on libmach but is producing
// an executable that can be parsed by tools like debuggers which rely
// on libmach to do so.

//******************************************************************************
// Executable header
//******************************************************************************

typedef	struct	Exec	Exec;
struct	Exec
{
	int32	magic;		/* magic number */

	int32	text;	 	/* size of text segment */
	int32	data;	 	/* size of initialized data */
	int32	bss;	  	/* size of uninitialized data */

	int32	syms;	 	/* size of symbol table */

	int32	entry;	 	/* entry point */

	int32	spsz;		/* size of pc/sp offset table */
	int32	pcsz;		/* size of pc/line number table */
};

#define HDR_MAGIC	0x00008000		/* header expansion */

#define	_MAGIC(f, b)	((f)|((((4*(b))+0)*(b))+7))
#define	I_MAGIC		_MAGIC(0, 11)		/* intel 386 */
#define	V_MAGIC		_MAGIC(0, 16)		/* mips 3000 BE */
#define	E_MAGIC		_MAGIC(0, 20)		/* arm */
#define	S_MAGIC		_MAGIC(HDR_MAGIC, 26)	/* amd64 */
#define	R_MAGIC		_MAGIC(HDR_MAGIC, 28)	/* arm64 */

#define	MIN_MAGIC	8
#define	MAX_MAGIC	28			/* <= 90 */

#define	DYN_MAGIC	0x80000000		/* dlm */

typedef	struct	Sym	Sym;
struct	Sym
{
	vlong	value;
	uint	sig;
	char	type;
	char	*name;
	vlong	gotype;
	int	sequence;	// order in file
};

//******************************************************************************
// Enums for the different archs (used to index Mach tables)
//******************************************************************************

/*
 *	Supported architectures:
 *		mips,
 *		i386,
 *		amd64,
 *		arm
 *      arm64
 *      TODO: riscv
 */
//TODO: split in different enums with different names so clearer
// (like in principia)
enum
{
    /* machine types */
	MMIPS = 0,			
	MI386,
	MARM,
	MAMD64,
    MARM64,

    /* types of executables */
	FNONE = 0,		/* unidentified */
	FMIPS,			/* v.out */
	FMIPSB,			/* mips bootable */
	FI386,			/* 8.out */
	FI386B,			/* I386 bootable */
	FARM,			/* 5.out */
	FARMB,			/* ARM bootable */
	FAMD64,			/* 6.out */
	FAMD64B,		/* 6.out bootable */
    FARM64, // arm64
    FARM64B, // arm64 bootable

    /* dissembler types */
	ANONE = 0,
	AMIPS,
	AI386,
	AARM,
	AAMD64,
    AARM64,

    /* object file types */
	ObjMips = 0,		/* .v */
	Obj386,			/* .8 */
	ObjArm,			/* .5 */
	ObjAmd64,		/* .6 */
    ObjArm64, // .7
	Maxobjtype,

    /* symbol table classes */
	CNONE  = 0,
	CAUTO,
	CPARAM,
	CSTAB,
	CTEXT,
	CDATA,
	CANY,			/* to look for any class */
};

//******************************************************************************
// Forward decls
//******************************************************************************

typedef	struct	Map	Map;
typedef	struct	Symbol	Symbol;
typedef	struct	Reglist	Reglist;
typedef	struct	Mach	Mach;
typedef	struct	Machdata Machdata;
typedef	struct	Seg	Seg;

//******************************************************************************
// Map and Seg
//******************************************************************************

typedef int Maprw(Map *m, Seg *s, uvlong addr, void *v, uint n, int isread);

struct Seg {
	char	*name;		/* the segment name */
	fdt	fd;		/* file descriptor */
	int	inuse;		/* in use - not in use */
	int	cache;		/* should cache reads? */
	uvlong	b;		/* base */
	uvlong	e;		/* end */
	vlong	f;		/* offset within file */
	Maprw	*rw;		/* read/write fn for seg */
};

/*
 * 	Structure to map a segment to data
 */
struct Map {
	int	pid;
	int	tid;
	int	nsegs;	/* number of segments */
	Seg	seg[1];	/* actually n of these */
};

/*
 *	Internal structure describing a symbol table entry
 */
struct Symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		vlong	value;		/* address or stack offset */
		char	type;		/* as in a.out.h */
		char	class;		/* as above */
		int	index;		/* in findlocal, globalsym, textsym */
	};
};

/*
 *	machine register description
 */
struct Reglist {
	char	*rname;			/* register name */
	short	roffs;			/* offset in u-block */
	char	rflags;			/* INTEGER/FLOAT, WRITABLE */
	char	rformat;		/* print format: 'x', 'X', 'f', '8', '3', 'Y', 'W' */
};

enum {					/* bits in rflags field */
	RINT	= (0<<0),
	RFLT	= (1<<0),
	RRDONLY	= (1<<1),
};

//******************************************************************************
// Mach and Machdata
//******************************************************************************

/*
 *	Machine-dependent data is stored in two structures:
 *		Mach  - miscellaneous general parameters
 *		Machdata - jump vector of service functions used by debuggers
 *
 *	Mach is defined in ${O}.c and set in executable.c
 *
 *	Machdata is defined in ${O}db.c
 *		and set in the debugger startup.
 */
struct Mach{
	char	*name;
	int	mtype;			/* machine type code */
	Reglist *reglist;		/* register set */
	int32	regsize;		/* sizeof registers in bytes */
	int32	fpregsize;		/* sizeof fp registers in bytes */
	char	*pc;			/* pc name */
	char	*sp;			/* sp name */
	char	*link;			/* link register name */
	char	*sbreg;			/* static base register name */
	uvlong	sb;			/* static base register value */
	int	pgsize;			/* page size */
	uvlong	kbase;			/* kernel base address */
	uvlong	ktmask;			/* ktzero = kbase & ~ktmask */
	uvlong	utop;			/* user stack top */
	int	pcquant;		/* quantization of pc */
	int	szaddr;			/* sizeof(void*) */
	int	szreg;			/* sizeof(register) */
	int	szfloat;		/* sizeof(float) */
	int	szdouble;		/* sizeof(double) */
};

// important global!
extern	Mach	*mach;			/* Current machine */

typedef uvlong	(*Rgetter)(Map*, char*);
typedef	void	(*Tracer)(Map*, uvlong, uvlong, Symbol*);

struct	Machdata {		/* Machine-dependent debugger support */
	uchar	bpinst[4];			/* break point instr. */
	short	bpsize;				/* size of break point instr. */

	ushort	(*swab)(ushort);		/* ushort to local byte order */
	uint32	(*swal)(uint32);			/* uint32 to local byte order */
	uvlong	(*swav)(uvlong);		/* uvlong to local byte order */
	int	(*ctrace)(Map*, uvlong, uvlong, uvlong, Tracer); /* C traceback */
	uvlong	(*findframe)(Map*, uvlong, uvlong, uvlong, uvlong);/* frame finder */
	char*	(*excep)(Map*, Rgetter);	/* last exception */
	uint32	(*bpfix)(uvlong);		/* breakpoint fixup */
	int	(*sftos)(char*, int, void*);	/* single precision float */
	int	(*dftos)(char*, int, void*);	/* double precision float */
	int	(*foll)(Map*, uvlong, Rgetter, uvlong*);/* follow set */
	int	(*das)(Map*, uvlong, char, char*, int);	/* symbolic disassembly */
	int	(*hexinst)(Map*, uvlong, char*, int); 	/* hex disassembly */
	int	(*instsize)(Map*, uvlong);	/* instruction size */
};

//******************************************************************************
// a.out header
//******************************************************************************

/*
 *	Common a.out header describing all architectures
 */
typedef struct Fhdr
{
	char	*name;		/* identifier of executable */

	uchar	type;		/* file type - see codes above */
	uchar	hdrsz;		/* header size */
	uchar	_magic;		/* _MAGIC() magic */
	uchar	spare;
	int32	magic;		/* magic number */

	uvlong	txtaddr;	/* text address */
	vlong	txtoff;		/* start of text in file */

	uvlong	dataddr;	/* start of data segment */
	vlong	datoff;		/* offset to data seg in file */

	vlong	symoff;		/* offset of symbol table in file */

	uvlong	entry;		/* entry point */

	vlong	sppcoff;	/* offset of sp-pc table in file */
	vlong	lnpcoff;	/* offset of line number-pc table in file */

	int32	txtsz;		/* text size */
	int32	datsz;		/* size of data seg */
	int32	bsssz;		/* size of bss */
	int32	symsz;		/* size of symbol table */
	int32	sppcsz;		/* size of sp-pc table */
	int32	lnpcsz;		/* size of line number-pc table */
} Fhdr;

//******************************************************************************
// Other globals
//******************************************************************************

extern	int	asstype;	/* dissembler type - machdata.c */
extern	Machdata *machdata;	/* jump vector - machdata.c */

//******************************************************************************
// API
//******************************************************************************

// executable.c:
// crackhdr

// obj.c: routines universal to all object files
// objtype, readobj, isar, readar, objtraverse, nextar

// setmach.c: 
// machines global, machbytype, machbyname

// map.c: file map routines
// newmap, setmap, findseg, unusedmap, loadmap
// fdrw, 

// sym.c: 
// syminit, textseg, symbase, getsym
// lookup, cdotstrcmp, findlocal, 
// textsym, filesym, getauto, findsym, fnbound, 
// localsym, globalsym
// file2pc, fileline, fileelem
// pc2sp, pc2line, line2addr, dumphist (ifdef DEBUG)

// access.c: functions to read and write an executable or file image
// geta, get8, get4, get2, get1
// puta, put8, put4, put2, put1

// machdata.c: Debugger utilities shared by at least two architectures
// localaddr, symoff, fpformat, 
// ieeedftos, ieeesftos, beieeesftos, beieeedftos, leieeesftos, leieeedftos
// beieee80ftos, leieee80ftos
// cisctrace, risctrace, ciscframe, riscframe

// swap.c: big/little endian
// beswab, beswal, beswav, leswab, leswal, leswav

int		beieee80ftos(char*, int, void*);
int		beieeesftos(char*, int, void*);
int		beieeedftos(char*, int, void*);

ushort		beswab(ushort);
uint32		beswal(uint32);
uvlong		beswav(uvlong);

uvlong		ciscframe(Map*, uvlong, uvlong, uvlong, uvlong);
int		cisctrace(Map*, uvlong, uvlong, uvlong, Tracer);

// important one!
int		crackhdr(fdt fd, Fhdr*);


uvlong		file2pc(char*, int32);
int		fileelem(Sym**, uchar *, char*, int);
int32		fileline(char*, int, uvlong);
int		filesym(int, char*, int);
int		findlocal(Symbol*, char*, Symbol*);
int		findseg(Map*, char*);
int		findsym(uvlong, int, Symbol *);
int		fnbound(uvlong, uvlong*);
int		fpformat(Map*, Reglist*, char*, int, int);

int		get1(Map*, uvlong, uchar*, int);
int		get2(Map*, uvlong, ushort*);
int		get4(Map*, uvlong, uint32*);
int		get8(Map*, uvlong, uvlong*);
int		geta(Map*, uvlong, uvlong*);

int		getauto(Symbol*, int, int, Symbol*);
Sym*		getsym(int);
int		globalsym(Symbol *, int);
char*		_hexify(char*, uint32, int);

int		ieeesftos(char*, int, uint32);
int		ieeedftos(char*, int, uint32, uint32);

int		isar(Biobuf*);

int		leieee80ftos(char*, int, void*);
int		leieeesftos(char*, int, void*);
int		leieeedftos(char*, int, void*);

ushort		leswab(ushort);
uint32		leswal(uint32);
uvlong		leswav(uvlong);

uvlong		line2addr(int32, uvlong, uvlong);
Map*		loadmap(Map*, int, Fhdr*);
int		localaddr(Map*, char*, char*, uvlong*, Rgetter);
int		localsym(Symbol*, int);

int		lookup(char*, char*, Symbol*);

void		machbytype(int);
int		machbyname(char*);

int		nextar(Biobuf*, int, char*);
Map*		newmap(Map*, int);
void		objtraverse(void(*)(Sym*, void*), void*);

// important one
int		objtype(Biobuf*, char**);

uvlong		pc2sp(uvlong);
int32		pc2line(uvlong);

int		put1(Map*, uvlong, uchar*, int);
int		put2(Map*, uvlong, ushort);
int		put4(Map*, uvlong, uint32);
int		put8(Map*, uvlong, uvlong);
int		puta(Map*, uvlong, uvlong);

int		readar(Biobuf*, int, vlong, int);
int		readobj(Biobuf*, int);
uvlong		riscframe(Map*, uvlong, uvlong, uvlong, uvlong);
int		risctrace(Map*, uvlong, uvlong, uvlong, Tracer);
int		setmap(Map*, int, uvlong, uvlong, vlong, char*, Maprw *rw);
Sym*		symbase(int32*);

int		syminit(int, Fhdr*);
int		symoff(char*, int, uvlong, int);

void		textseg(uvlong, Fhdr*);
int		textsym(Symbol*, int);

void		unusemap(Map*, int);

// proc
Map*	attachproc(pidt pid, Fhdr *fp);
int		ctlproc(int pid, char *msg);
void	detachproc(Map *m);
int		procnotes(int pid, char ***pnotes);
char*	proctextfile(int pid);
int		procthreadpids(int pid, int *tid, int ntid);
char*	procstatus(int);

// ??
Maprw	fdrw;
