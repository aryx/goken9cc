/*
 * claude: the kencc arm lineage (5ak/5ck/5lk) now shares the
 * principia-synced object format header (include/5.out.h), so both
 * lineages serialize identical .5 object files (same reconciliation
 * as x86, but adopting the principia numbering instead of plan9's:
 * the principia books reorganized the arm enums and the rest of the
 * ecosystem follows them).
 *
 * This wrapper adds back the identifiers the principia header dropped,
 * without disturbing any shared serialized value:
 *  - opcodes unused by the principia toolchain (dynamic linking,
 *    thumb, load/store-exclusive, DWORD), appended after ALAST; they
 *    never appear in objects compared against the principia lineage
 *  - ABCS/ABCC, plan9 synonyms of ABHS/ABLO (same condition codes,
 *    so they alias to the same serialized values)
 *  - the plan9 names for the symbol kinds (D_EXTERN etc.), aliased
 *    to the principia N_* enumerators with identical semantics
 *  - D_OCONST, a linker-internal operand kind, appended after D_PSR
 *  - the thumb-support constants (REGTMPT, ALLTHUMBS)
 */
#include "../../include/obj/5.out.h"
#include "../../include/obj/common.out.h"	/* NSYM, Ieee */

enum Opcode_kencc
{
	ADYNT = ALAST,
	AINIT,

	ABX,
	ABXRET,
	ADWORD,

	ALDREX,
	ASTREX,
	ALDREXD,
	ASTREXD,

	ALAST_KENCC,
};
#define	ALAST	ALAST_KENCC

/* plan9 synonyms for the unsigned-compare conditional branches */
#define	ABCS	ABHS
#define	ABCC	ABLO

/* plan9 names for the symbol kinds */
#define	D_EXTERN	N_EXTERN
#define	D_STATIC	N_INTERN
#define	D_AUTO		N_LOCAL
#define	D_PARAM		N_PARAM
#define	D_FILE		N_FILE
#define	D_FILE1		N_LINE

enum Operand_kind_kencc
{
	D_OCONST = D_PSR+1,
};

#define	REGTMPT		7	/* used by the loader for thumb code */
#define	ALLTHUMBS	(1<<2)
