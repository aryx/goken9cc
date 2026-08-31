#define	NSNAME	8
#define	NSYM	50
#define	NREG	32
#define NOPROF	(1<<0)
#define DUPOK	(1<<1)

enum
{
	REGZERO		= 0,	/* always zero */
	REGSP		= 1,	/* stack pointer */
	REGSB		= 2,	/* static pointer */
	REGSB1		= 3,	/* (possible) second static pointer */
	REGEXT		= 6,	/* first external register, grows-- */
	REGRET		= 7,	/* return register and first temp, grows++ */
	REGTMP		= 14,	/* used by the loader */
	REGLINK		= 15,	/* subroutine linkage */
	REGARG		= 7,	/* first arg passed in */

	FREGRET		= 0,
	FREGEXT		= 22,	/* first external register */
	FREGZERO	= 24,	/* both float and double */
	FREGHALF	= 26,	/* double */
	FREGONE		= 28,	/* double */
	FREGTWO		= 30	/* double */
/*
 * GENERAL:
 *
 * compiler allocates R7 up as temps
 * compiler allocates external registers R6 down
 * compiler allocates register variables F4-F22
 * compiler allocates external registers F22 down
 */
};

enum	as
{
	AXXX	= 0,
	AADD,
	AADDCC,
	AADDX,
	AADDXCC,
	AAND,
	AANDCC,
	AANDN,
	AANDNCC,
	ABA,
	ABCC,
	ABCS,
	ABE,
	ABG,
	ABGE,
	ABGU,
	ABL,
	ABLE,
	ABLEU,
	ABN,
	ABNE,
	ABNEG,
	ABPOS,
	ABVC,
	ABVS,
	ACB0,
	ACB01,
	ACB012,
	ACB013,
	ACB02,
	ACB023,
	ACB03,
	ACB1,
	ACB12,
	ACB123,
	ACB13,
	ACB2,
	ACB23,
	ACB3,
	ACBA,
	ACBN,
	ACMP,		/* pseudo op */
	ACPOP1,
	ACPOP2,
	ADATA,
	ADIV,
	ADIVL,
	AFABSD,		/* pseudo op */
	AFABSF,
	AFABSX,		/* pseudo op */
	AFADDD,
	AFADDF,
	AFADDX,
	AFBA,
	AFBE,
	AFBG,
	AFBGE,
	AFBL,
	AFBLE,
	AFBLG,
	AFBN,
	AFBNE,
	AFBO,
	AFBU,
	AFBUE,
	AFBUG,
	AFBUGE,
	AFBUL,
	AFBULE,
	AFCMPD,
	AFCMPED,
	AFCMPEF,
	AFCMPEX,
	AFCMPF,
	AFCMPX,
	AFDIVD,
	AFDIVF,
	AFDIVX,
	AFMOVD,		/* pseudo op */
	AFMOVDF,
	AFMOVDW,
	AFMOVDX,
	AFMOVF,
	AFMOVFD,
	AFMOVFW,
	AFMOVFX,
	AFMOVWD,
	AFMOVWF,
	AFMOVWX,
	AFMOVX,		/* pseudo op */
	AFMOVXD,
	AFMOVXF,
	AFMOVXW,
	AFMULD,
	AFMULF,
	AFMULX,
	AFNEGD,		/* pseudo op */
	AFNEGF,
	AFNEGX,		/* pseudo op */
	AFSQRTD,
	AFSQRTF,
	AFSQRTX,
	AFSUBD,
	AFSUBF,
	AFSUBX,
	AGLOBL,
	AGOK,
	AHISTORY,
	AIFLUSH,
	AJMPL,
	AJMP,
	AMOD,
	AMODL,
	AMOVB,
	AMOVBU,
	AMOVD,
	AMOVH,
	AMOVHU,
	AMOVW,
	AMUL,
	AMULSCC,
	ANAME,
	ANOP,
	AOR,
	AORCC,
	AORN,
	AORNCC,
	ARESTORE,
	ARETT,
	ARETURN,
	ASAVE,
	ASLL,
	ASRA,
	ASRL,
	ASUB,
	ASUBCC,
	ASUBX,
	ASUBXCC,
	ASWAP,
	ATA,
	ATADDCC,
	ATADDCCTV,
	ATAS,
	ATCC,
	ATCS,
	ATE,
	ATEXT,
	ATG,
	ATGE,
	ATGU,
	ATL,
	ATLE,
	ATLEU,
	ATN,
	ATNE,
	ATNEG,
	ATPOS,
	ATSUBCC,
	ATSUBCCTV,
	ATVC,
	ATVS,
	AUNIMP,
	AWORD,
	AXNOR,
	AXNORCC,
	AXOR,
	AXORCC,
	AEND,
	ADYNT,
	AINIT,
	ASIGNAME,
	ALAST
};

/* type/name */
enum
{
	D_GOK	= 0,
	D_NONE,

/* name */
	D_EXTERN,
	D_STATIC,
	D_AUTO,
	D_PARAM,

/* type */
	D_BRANCH,
	D_OREG,
	D_ASI,
	D_CONST,
	D_FCONST,
	D_SCONST,
	D_REG,
	D_FREG,
	D_CREG,
	D_PREG,
	D_FILE,
	D_FILE1,

/* reg names iff type is D_PREG */
	D_CPQ	= 0,
	D_CSR,
	D_FPQ,
	D_FSR,
	D_PSR,
	D_TBR,
	D_WIM,
	D_Y
};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

/*
 * this is the simulated IEEE floating point
 */
typedef	struct	ieee	Ieee;
struct	ieee
{
	/* claude: int32, not long -- kl/asm.c's data writer treats
	 * &p->to.ieee as a tightly-packed 8-byte blob (two 4-byte
	 * halves), and kl/obj.c's reader assigns each half from a
	 * byte-shifted 32-bit expression that can be negative (e.g. a
	 * mantissa byte with the top bit set). A `long` field here is
	 * 8 bytes on a 64-bit host, so that negative 32-bit value
	 * sign-extends into the upper 4 bytes instead of being
	 * confined to them -- corrupting every FCONST double whose low
	 * mantissa byte has bit 7 set (found via 2.2's %f coming out as
	 * NaN: the read side produced ieee.l=0xffffffff9999999a instead
	 * of 0x9999999a). See q.out.h/v.out.h/i.out.h/6.out.h, which
	 * already use int32 here. */
	int32	l;	/* contains ls-man	0xffffffff */
	int32	h;	/* contains sign	0x80000000
				    exp		0x7ff00000
				    ms-man	0x000fffff */
};
