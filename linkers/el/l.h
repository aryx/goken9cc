/*
 * el/l.h -- wasm linker
 *
 * claude: modeled on linkers/5l/l.h's shape (Sym/Prog/Adr, obj.c
 * reads into a Prog list, asm.c emits the final binary), but scaled
 * way down: no archives (-l libs), no DUPOK-merge/auto-param/history/
 * debugging/profiling/dynamic-relocation machinery -- none of that is
 * needed to link one object file's worth of straight-line code into a
 * module a wasm host can run. Extending this the way 5l grew (many
 * arches, many object files, archives) is future work, not a redesign.
 */
#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#include	<common.out.h>
#include	<e.out.h>

typedef struct	Adr	Adr;
typedef struct	Instr	Instr;
typedef struct	Sym	Sym;
typedef struct	Text	Text;
typedef struct	Import	Import;

/* claude: mirrors assemblers/ea/a.h's Gen exactly -- the wire format
 * outopd() wrote is read back into this same shape by inopd() below. */
struct	Adr
{
    short	type;	/* D_* "type" */
    short	name;	/* D_* "name", meaningful only when type==D_OREG */
    long	offset;
    vlong	vval;	/* D_VCONST */
    double	dval;	/* D_FCONST */
    char	sval[NSNAME];	/* D_SCONST */
    Sym*	sym;
};

struct	Instr
{
    int		as;	/* enum<as> */
    Adr		from;
    Adr		to;
    Instr*	link;
};

enum Section
{
    SNONE = 0,
    STEXT,		/* a defined function (from ATEXT) */
    SBSS,		/* a GLOBL'd data symbol, byte content in data[] */
    SIMPORT,	/* resolved via a -I flag to a host import */
    SXREF,		/* referenced (e.g. by CALL) but not yet resolved */
};

struct	Sym
{
    char*	name;
    short	type;	/* enum Section */

    /* SBSS: linear-memory byte content, grown by DATA/GLOBL records.
     * datasize is the logical/declared size asm.c's layout uses;
     * datacap is data[]'s actual allocation, grown geometrically and
     * otherwise unrelated to datasize -- see obj.c's growdata(). */
    char*	data;
    long	datasize;
    long	datacap;

    /* resolved by asm.c's layout pass */
    long	value;	/* STEXT/SIMPORT: function index. SBSS: arena offset */

    Sym*	link;	/* hash chain */
    Sym*	link2;	/* asm.c's collectsyms(): SBSS symbols only */
};
#define	S	((Sym*)nil)

struct	Text
{
    Sym*	sym;
    long	framesize;
    Instr*	first;
    Instr*	last;
    Text*	link;
};

/* claude: one entry per -I flag: `-I symbol=module.field` tells el
 * that an otherwise-undefined CALL target should become a wasm
 * import instead of a link error, the same role 6lg's `-I thunk:sym:
 * lib` plays for Mach-O dynamic imports (see tests/s/mini/mkfile's
 * hello_macos_libc_amd64 recipe) -- adapted to wasm's two-level
 * (module, field) import naming instead of a library path. v1 only
 * supports one hardcoded signature (4 x i32 -> i32, i.e. WASI's
 * fd_write) -- see asm.c's importsig comment.
 */
struct	Import
{
    char*	symname;
    char*	module;
    char*	field;
    Import*	link;
};

/*
 * claude: a DATA value that is itself a symbol's address (`DATA
 * iov+0(SB)/4, $msg(SB)`) can't be patched into targetsym->data while
 * reading the object file: msg's own arena address isn't known until
 * asm.c's layout pass has assigned one to every symbol. Real linkers
 * solve the general version of this (forward/circular refs *across
 * object files*, plus branch-distance feedback loops) with a full
 * relocation table; el only reads one file and wasm branches are
 * depth- not distance-based, so all that's needed here is: lay out
 * every symbol first, then apply this short deferred-patch list,
 * then emit code (which can look up already-resolved sym->value
 * directly, no relocation bookkeeping needed on the code side at all).
 */
struct	DataReloc
{
    Sym*	targetsym;
    long	targetoff;
    int	width;
    Sym*	refsym;
    long	addend;
    struct DataReloc* link;
};
typedef struct DataReloc DataReloc;

#define	NHASH	1009

extern	Sym*	hash[NHASH];
extern	Text*	firsttext;
extern	Text*	lasttext;
extern	Text*	curtext;
extern	Import*	imports;
extern	DataReloc*	datarelocs;
extern	char*	outfile;
extern	Biobuf	obuf;
extern	int	nerrors;

Sym*	lookup(char*);
Text*	newtext(Sym*);
Instr*	newinstr(int, Adr*, Adr*);
void	addimport(char*, char*, char*);

void	readobj(char*);

void	asmb(void);

/*
 * optab.c: one real wasm opcode encoding per e.out.h opcode -- the
 * counterpart of every other arch's Optab/oplook(), simplified since
 * wasm has no addressing-mode-dependent variants of the same
 * instruction to classify between (a real arch's oplook() picks among
 * several Optab rows per opcode via aclass(); here every opcode has
 * exactly one encoding, so the table is keyed by opcode alone).
 */
enum
{
    OSIMPLE,	/* one fixed byte, no operand */
    OSIMPLE2,	/* two fixed bytes (opcode + a required immediate byte) */
    OMOVE,	/* virtual AMOVx: const/load/store, or a bare conversion */
    OBR,	/* br/br_if: opcode + uleb depth */
    OCALL,	/* call: opcode + resolved function index */
    OLOCAL,	/* local.tee: opcode + uleb local index */
};

typedef struct	Optab	Optab;
struct	Optab
{
    int	as;
    int	kind;
    int	op;		/* primary wasm opcode byte */
    int	op2;		/* OSIMPLE2's required immediate byte */
    int	loadop;		/* OMOVE: load variant, -1 if none (pure conversion) */
    int	storeop;	/* OMOVE: store variant, -1 if none (pure conversion) */
    int	constkind;	/* OMOVE: 'w'/'q'/'f'/'d', which const/push applies */
};

Optab*	oplook(int);

void	diag(char*, ...);
void	errorexit(void);
