/*
 * el/optab.c -- one row per e.out.h opcode, giving its real wasm
 * encoding. See l.h's Optab comment for how this differs from a real
 * arch's optab.c: no operand-class dispatch, because a wasm opcode
 * has exactly one encoding (the operand *kind* -- const/memory/local/
 * global -- is already baked into which opcode ea chose, not
 * something asm.c has to classify the way aclass() does elsewhere).
 *
 * The byte values are the WASM MVP spec's instruction encodings
 * (stable/standardized -- e.g. https://webassembly.github.io/spec/
 * core/binary/instructions.html), not something this project invented.
 */
#include "l.h"

Optab optab[] =
{
/*	as		kind		op	op2	fsize */
	AXXX,		OSIMPLE,	-1,	0,	0,

	ANOP,		OSIMPLE,	0x01,	0,	0,
	AUNREACHABLE,	OSIMPLE,	0x00,	0,	0,
	ABLOCK,		OSIMPLE2,	0x02,	0x40,	0,
	ALOOP,		OSIMPLE2,	0x03,	0x40,	0,
	AIF,		OSIMPLE2,	0x04,	0x40,	0,
	AELSE,		OSIMPLE,	0x05,	0,	0,
	AENDCTL,	OSIMPLE,	0x0B,	0,	0,
	ABR,		OBR,		0x0C,	0,	0,
	ABRIF,		OBR,		0x0D,	0,	0,
	ARET,		OSIMPLE,	0x0F,	0,	0,
	ACALL,		OCALL,		0x10,	0,	0,
	ADROP,		OSIMPLE,	0x1A,	0,	0,
	ASELECT,	OSIMPLE,	0x1B,	0,	0,
	AMEMSIZE,	OSIMPLE2,	0x3F,	0x00,	0,
	AMEMGROW,	OSIMPLE2,	0x40,	0x00,	0,

	ALOCALGET,	OLOCAL,		0x20,	0,	0,
	ALOCALSET,	OLOCAL,		0x21,	0,	0,
	ALOCALTEE,	OLOCAL,		0x22,	0,	0,
	AGLOBALGET,	OGLOBAL,	0x23,	0,	0,
	AGLOBALSET,	OGLOBAL,	0x24,	0,	0,

	ACONSTW,	OCONSTI,	0x41,	0,	0,
	ACONSTQ,	OCONSTI,	0x42,	0,	0,
	ACONSTF,	OCONSTF,	0x43,	0,	4,
	ACONSTD,	OCONSTF,	0x44,	0,	8,

	ALOADB,		OMEM,		0x2C,	0,	0,
	ALOADBU,	OMEM,		0x2D,	0,	0,
	ALOADH,		OMEM,		0x2E,	0,	0,
	ALOADHU,	OMEM,		0x2F,	0,	0,
	ALOADW,		OMEM,		0x28,	0,	0,
	ALOADQ,		OMEM,		0x29,	0,	0,
	ALOADBQ,	OMEM,		0x30,	0,	0,
	ALOADBUQ,	OMEM,		0x31,	0,	0,
	ALOADHQ,	OMEM,		0x32,	0,	0,
	ALOADHUQ,	OMEM,		0x33,	0,	0,
	ALOADWQ,	OMEM,		0x34,	0,	0,
	ALOADWUQ,	OMEM,		0x35,	0,	0,
	ALOADF,		OMEM,		0x2A,	0,	0,
	ALOADD,		OMEM,		0x2B,	0,	0,

	ASTOREB,	OMEM,		0x3A,	0,	0,
	ASTOREH,	OMEM,		0x3B,	0,	0,
	ASTOREW,	OMEM,		0x36,	0,	0,
	ASTOREWQ,	OMEM,		0x3E,	0,	0,
	ASTOREQ,	OMEM,		0x37,	0,	0,
	ASTOREF,	OMEM,		0x38,	0,	0,
	ASTORED,	OMEM,		0x39,	0,	0,

	/* i32 arithmetic/logic/compare */
	AADDW,		OSIMPLE,	0x6A,	0,	0,
	ASUBW,		OSIMPLE,	0x6B,	0,	0,
	AMULW,		OSIMPLE,	0x6C,	0,	0,
	ADIVW,		OSIMPLE,	0x6D,	0,	0,
	ADIVWU,		OSIMPLE,	0x6E,	0,	0,
	AREMW,		OSIMPLE,	0x6F,	0,	0,
	AREMWU,		OSIMPLE,	0x70,	0,	0,
	AANDW,		OSIMPLE,	0x71,	0,	0,
	AORW,		OSIMPLE,	0x72,	0,	0,
	AXORW,		OSIMPLE,	0x73,	0,	0,
	ASHLW,		OSIMPLE,	0x74,	0,	0,
	ASHRW,		OSIMPLE,	0x75,	0,	0,
	ASHRWU,		OSIMPLE,	0x76,	0,	0,
	AROLW,		OSIMPLE,	0x77,	0,	0,
	ARORW,		OSIMPLE,	0x78,	0,	0,
	ACLZW,		OSIMPLE,	0x67,	0,	0,
	ACTZW,		OSIMPLE,	0x68,	0,	0,
	APOPCNTW,	OSIMPLE,	0x69,	0,	0,
	ATESTW,		OSIMPLE,	0x45,	0,	0,
	ACMPEQW,	OSIMPLE,	0x46,	0,	0,
	ACMPNEW,	OSIMPLE,	0x47,	0,	0,
	ACMPLTW,	OSIMPLE,	0x48,	0,	0,
	ACMPLTWU,	OSIMPLE,	0x49,	0,	0,
	ACMPGTW,	OSIMPLE,	0x4A,	0,	0,
	ACMPGTWU,	OSIMPLE,	0x4B,	0,	0,
	ACMPLEW,	OSIMPLE,	0x4C,	0,	0,
	ACMPLEWU,	OSIMPLE,	0x4D,	0,	0,
	ACMPGEW,	OSIMPLE,	0x4E,	0,	0,
	ACMPGEWU,	OSIMPLE,	0x4F,	0,	0,

	/* i64 arithmetic/logic/compare */
	AADDQ,		OSIMPLE,	0x7C,	0,	0,
	ASUBQ,		OSIMPLE,	0x7D,	0,	0,
	AMULQ,		OSIMPLE,	0x7E,	0,	0,
	ADIVQ,		OSIMPLE,	0x7F,	0,	0,
	ADIVQU,		OSIMPLE,	0x80,	0,	0,
	AREMQ,		OSIMPLE,	0x81,	0,	0,
	AREMQU,		OSIMPLE,	0x82,	0,	0,
	AANDQ,		OSIMPLE,	0x83,	0,	0,
	AORQ,		OSIMPLE,	0x84,	0,	0,
	AXORQ,		OSIMPLE,	0x85,	0,	0,
	ASHLQ,		OSIMPLE,	0x86,	0,	0,
	ASHRQ,		OSIMPLE,	0x87,	0,	0,
	ASHRQU,		OSIMPLE,	0x88,	0,	0,
	AROLQ,		OSIMPLE,	0x89,	0,	0,
	ARORQ,		OSIMPLE,	0x8A,	0,	0,
	ACLZQ,		OSIMPLE,	0x79,	0,	0,
	ACTZQ,		OSIMPLE,	0x7A,	0,	0,
	APOPCNTQ,	OSIMPLE,	0x7B,	0,	0,
	ATESTQ,		OSIMPLE,	0x50,	0,	0,
	ACMPEQQ,	OSIMPLE,	0x51,	0,	0,
	ACMPNEQ,	OSIMPLE,	0x52,	0,	0,
	ACMPLTQ,	OSIMPLE,	0x53,	0,	0,
	ACMPLTQU,	OSIMPLE,	0x54,	0,	0,
	ACMPGTQ,	OSIMPLE,	0x55,	0,	0,
	ACMPGTQU,	OSIMPLE,	0x56,	0,	0,
	ACMPLEQ,	OSIMPLE,	0x57,	0,	0,
	ACMPLEQU,	OSIMPLE,	0x58,	0,	0,
	ACMPGEQ,	OSIMPLE,	0x59,	0,	0,
	ACMPGEQU,	OSIMPLE,	0x5A,	0,	0,

	/* f32 arithmetic/compare */
	AADDF,		OSIMPLE,	0x92,	0,	0,
	ASUBF,		OSIMPLE,	0x93,	0,	0,
	AMULF,		OSIMPLE,	0x94,	0,	0,
	ADIVF,		OSIMPLE,	0x95,	0,	0,
	AMINF,		OSIMPLE,	0x96,	0,	0,
	AMAXF,		OSIMPLE,	0x97,	0,	0,
	ACOPYSGNF,	OSIMPLE,	0x98,	0,	0,
	AABSF,		OSIMPLE,	0x8B,	0,	0,
	ANEGF,		OSIMPLE,	0x8C,	0,	0,
	ASQRTF,		OSIMPLE,	0x91,	0,	0,
	ACEILF,		OSIMPLE,	0x8D,	0,	0,
	AFLOORF,	OSIMPLE,	0x8E,	0,	0,
	ATRUNCF,	OSIMPLE,	0x8F,	0,	0,
	ANEARF,		OSIMPLE,	0x90,	0,	0,
	ACMPEQF,	OSIMPLE,	0x5B,	0,	0,
	ACMPNEF,	OSIMPLE,	0x5C,	0,	0,
	ACMPLTF,	OSIMPLE,	0x5D,	0,	0,
	ACMPGTF,	OSIMPLE,	0x5E,	0,	0,
	ACMPLEF,	OSIMPLE,	0x5F,	0,	0,
	ACMPGEF,	OSIMPLE,	0x60,	0,	0,

	/* f64 arithmetic/compare */
	AADDD,		OSIMPLE,	0xA0,	0,	0,
	ASUBD,		OSIMPLE,	0xA1,	0,	0,
	AMULD,		OSIMPLE,	0xA2,	0,	0,
	ADIVD,		OSIMPLE,	0xA3,	0,	0,
	AMIND,		OSIMPLE,	0xA4,	0,	0,
	AMAXD,		OSIMPLE,	0xA5,	0,	0,
	ACOPYSGND,	OSIMPLE,	0xA6,	0,	0,
	AABSD,		OSIMPLE,	0x99,	0,	0,
	ANEGD,		OSIMPLE,	0x9A,	0,	0,
	ASQRTD,		OSIMPLE,	0x9F,	0,	0,
	ACEILD,		OSIMPLE,	0x9B,	0,	0,
	AFLOORD,	OSIMPLE,	0x9C,	0,	0,
	ATRUNCD,	OSIMPLE,	0x9D,	0,	0,
	ANEARD,		OSIMPLE,	0x9E,	0,	0,
	ACMPEQD,	OSIMPLE,	0x61,	0,	0,
	ACMPNED,	OSIMPLE,	0x62,	0,	0,
	ACMPLTD,	OSIMPLE,	0x63,	0,	0,
	ACMPGTD,	OSIMPLE,	0x64,	0,	0,
	ACMPLED,	OSIMPLE,	0x65,	0,	0,
	ACMPGED,	OSIMPLE,	0x66,	0,	0,

	/* register-only conversions, zero operands (see e.out.h's comment) */
	AWRAPQ,		OSIMPLE,	0xA7,	0,	0,
	AEXTW,		OSIMPLE,	0xAC,	0,	0,
	AEXTWU,		OSIMPLE,	0xAD,	0,	0,
	ACONVWF,	OSIMPLE,	0xB2,	0,	0,
	ACONVWUF,	OSIMPLE,	0xB3,	0,	0,
	ACONVWD,	OSIMPLE,	0xB7,	0,	0,
	ACONVWUD,	OSIMPLE,	0xB8,	0,	0,
	ACONVQF,	OSIMPLE,	0xB4,	0,	0,
	ACONVQUF,	OSIMPLE,	0xB5,	0,	0,
	ACONVQD,	OSIMPLE,	0xB9,	0,	0,
	ACONVQUD,	OSIMPLE,	0xBA,	0,	0,
	ATRUNCFW,	OSIMPLE,	0xA8,	0,	0,
	ATRUNCFWU,	OSIMPLE,	0xA9,	0,	0,
	ATRUNCFQ,	OSIMPLE,	0xAE,	0,	0,
	ATRUNCFQU,	OSIMPLE,	0xAF,	0,	0,
	ATRUNCDW,	OSIMPLE,	0xAA,	0,	0,
	ATRUNCDWU,	OSIMPLE,	0xAB,	0,	0,
	ATRUNCDQ,	OSIMPLE,	0xB0,	0,	0,
	ATRUNCDQU,	OSIMPLE,	0xB1,	0,	0,
	APROMOTE,	OSIMPLE,	0xBB,	0,	0,
	ADEMOTE,	OSIMPLE,	0xB6,	0,	0,
	AREINTWF,	OSIMPLE,	0xBE,	0,	0,
	AREINTFW,	OSIMPLE,	0xBC,	0,	0,
	AREINTQD,	OSIMPLE,	0xBF,	0,	0,
	AREINTDQ,	OSIMPLE,	0xBD,	0,	0,

	ALAST,		OSIMPLE,	-1,	0,	0,
};

Optab*
oplook(int as)
{
    Optab *o;

    for(o = optab; o->as != ALAST; o++)
        if(o->as == as)
            return o;
    diag("oplook: opcode %d not in optab (missing from ea's grammar too?)", as);
    errorexit();
    return nil;
}
