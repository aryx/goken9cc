/*
 * el/optab.c -- one row per e.out.h opcode, giving its real wasm
 * encoding. See l.h's Optab comment for how this differs from a real
 * arch's optab.c: no operand-class dispatch, because a wasm opcode
 * has exactly one encoding (the operand *kind* -- const/memory/local/
 * global -- selects a load/store/const variant, handled by asm.c's
 * fetch()/store(), not by picking a different Optab row here).
 *
 * The byte values are the WASM MVP spec's instruction encodings
 * (stable/standardized -- e.g. https://webassembly.github.io/spec/
 * core/binary/instructions.html), not something this project invented.
 */
#include "l.h"

Optab optab[] =
{
/*	as		kind		op	op2	loadop	storeop	constkind */
	AXXX,		OSIMPLE,	-1,	0,	-1,	-1,	0,

	ANOP,		OSIMPLE,	0x01,	0,	-1,	-1,	0,
	AUNREACHABLE,	OSIMPLE,	0x00,	0,	-1,	-1,	0,
	ABLOCK,		OSIMPLE2,	0x02,	0x40,	-1,	-1,	0,
	ALOOP,		OSIMPLE2,	0x03,	0x40,	-1,	-1,	0,
	AIF,		OSIMPLE2,	0x04,	0x40,	-1,	-1,	0,
	AELSE,		OSIMPLE,	0x05,	0,	-1,	-1,	0,
	AENDCTL,	OSIMPLE,	0x0B,	0,	-1,	-1,	0,
	ABR,		OBR,		0x0C,	0,	-1,	-1,	0,
	ABRIF,		OBR,		0x0D,	0,	-1,	-1,	0,
	ARET,		OSIMPLE,	0x0F,	0,	-1,	-1,	0,
	ACALL,		OCALL,		0x10,	0,	-1,	-1,	0,
	ADROP,		OSIMPLE,	0x1A,	0,	-1,	-1,	0,
	ASELECT,	OSIMPLE,	0x1B,	0,	-1,	-1,	0,
	ALOCALTEE,	OLOCAL,		0x22,	0,	-1,	-1,	0,
	AMEMSIZE,	OSIMPLE2,	0x3F,	0x00,	-1,	-1,	0,
	AMEMGROW,	OSIMPLE2,	0x40,	0x00,	-1,	-1,	0,

	/* virtual AMOVx: width-family (const/load/store) */
	AMOVB,		OMOVE,		0,	0,	0x2C,	0x3A,	'w',
	AMOVBU,		OMOVE,		0,	0,	0x2D,	0x3A,	'w',
	AMOVH,		OMOVE,		0,	0,	0x2E,	0x3B,	'w',
	AMOVHU,		OMOVE,		0,	0,	0x2F,	0x3B,	'w',
	AMOVW,		OMOVE,		0,	0,	0x28,	0x36,	'w',
	AMOVQ,		OMOVE,		0,	0,	0x29,	0x37,	'q',
	AMOVF,		OMOVE,		0,	0,	0x2A,	0x38,	'f',
	AMOVD,		OMOVE,		0,	0,	0x2B,	0x39,	'd',
	AMOVBQ,		OMOVE,		0,	0,	0x30,	0x3C,	'q',
	AMOVBUQ,	OMOVE,		0,	0,	0x31,	0x3C,	'q',
	AMOVHQ,		OMOVE,		0,	0,	0x32,	0x3D,	'q',
	AMOVHUQ,	OMOVE,		0,	0,	0x33,	0x3D,	'q',
	AMOVWQ,		OMOVE,		0,	0,	0x34,	0x3E,	'q',
	AMOVWUQ,	OMOVE,		0,	0,	0x35,	0x3E,	'q',

	/* virtual AMOVx: pure register conversions -- no load/store variant */
	AMOVQW,		OMOVE,		0xA7,	0,	-1,	-1,	'q',
	AMOVWF,		OMOVE,		0xB2,	0,	-1,	-1,	'w',
	AMOVWUF,	OMOVE,		0xB3,	0,	-1,	-1,	'w',
	AMOVWD,		OMOVE,		0xB7,	0,	-1,	-1,	'w',
	AMOVWUD,	OMOVE,		0xB8,	0,	-1,	-1,	'w',
	AMOVQF,		OMOVE,		0xB4,	0,	-1,	-1,	'q',
	AMOVQUF,	OMOVE,		0xB5,	0,	-1,	-1,	'q',
	AMOVQD,		OMOVE,		0xB9,	0,	-1,	-1,	'q',
	AMOVQUD,	OMOVE,		0xBA,	0,	-1,	-1,	'q',
	AMOVFW,		OMOVE,		0xA8,	0,	-1,	-1,	'f',
	AMOVFWU,	OMOVE,		0xA9,	0,	-1,	-1,	'f',
	AMOVFQ,		OMOVE,		0xAE,	0,	-1,	-1,	'f',
	AMOVFQU,	OMOVE,		0xAF,	0,	-1,	-1,	'f',
	AMOVDW,		OMOVE,		0xAA,	0,	-1,	-1,	'd',
	AMOVDWU,	OMOVE,		0xAB,	0,	-1,	-1,	'd',
	AMOVDQ,		OMOVE,		0xB0,	0,	-1,	-1,	'd',
	AMOVDQU,	OMOVE,		0xB1,	0,	-1,	-1,	'd',
	AMOVFD,		OMOVE,		0xBB,	0,	-1,	-1,	'f',
	AMOVDF,		OMOVE,		0xB6,	0,	-1,	-1,	'd',
	AREINTWF,	OMOVE,		0xBE,	0,	-1,	-1,	'w',
	AREINTFW,	OMOVE,		0xBC,	0,	-1,	-1,	'f',
	AREINTQD,	OMOVE,		0xBF,	0,	-1,	-1,	'q',
	AREINTDQ,	OMOVE,		0xBD,	0,	-1,	-1,	'd',

	/* i32 arithmetic/logic/compare */
	AADDW,		OSIMPLE,	0x6A,	0,	-1,	-1,	0,
	ASUBW,		OSIMPLE,	0x6B,	0,	-1,	-1,	0,
	AMULW,		OSIMPLE,	0x6C,	0,	-1,	-1,	0,
	ADIVW,		OSIMPLE,	0x6D,	0,	-1,	-1,	0,
	ADIVWU,		OSIMPLE,	0x6E,	0,	-1,	-1,	0,
	AREMW,		OSIMPLE,	0x6F,	0,	-1,	-1,	0,
	AREMWU,		OSIMPLE,	0x70,	0,	-1,	-1,	0,
	AANDW,		OSIMPLE,	0x71,	0,	-1,	-1,	0,
	AORW,		OSIMPLE,	0x72,	0,	-1,	-1,	0,
	AXORW,		OSIMPLE,	0x73,	0,	-1,	-1,	0,
	ASHLW,		OSIMPLE,	0x74,	0,	-1,	-1,	0,
	ASHRW,		OSIMPLE,	0x75,	0,	-1,	-1,	0,
	ASHRWU,		OSIMPLE,	0x76,	0,	-1,	-1,	0,
	AROLW,		OSIMPLE,	0x77,	0,	-1,	-1,	0,
	ARORW,		OSIMPLE,	0x78,	0,	-1,	-1,	0,
	ACLZW,		OSIMPLE,	0x67,	0,	-1,	-1,	0,
	ACTZW,		OSIMPLE,	0x68,	0,	-1,	-1,	0,
	APOPCNTW,	OSIMPLE,	0x69,	0,	-1,	-1,	0,
	ATESTW,		OSIMPLE,	0x45,	0,	-1,	-1,	0,
	ACMPEQW,	OSIMPLE,	0x46,	0,	-1,	-1,	0,
	ACMPNEW,	OSIMPLE,	0x47,	0,	-1,	-1,	0,
	ACMPLTW,	OSIMPLE,	0x48,	0,	-1,	-1,	0,
	ACMPLTWU,	OSIMPLE,	0x49,	0,	-1,	-1,	0,
	ACMPGTW,	OSIMPLE,	0x4A,	0,	-1,	-1,	0,
	ACMPGTWU,	OSIMPLE,	0x4B,	0,	-1,	-1,	0,
	ACMPLEW,	OSIMPLE,	0x4C,	0,	-1,	-1,	0,
	ACMPLEWU,	OSIMPLE,	0x4D,	0,	-1,	-1,	0,
	ACMPGEW,	OSIMPLE,	0x4E,	0,	-1,	-1,	0,
	ACMPGEWU,	OSIMPLE,	0x4F,	0,	-1,	-1,	0,

	/* i64 arithmetic/logic/compare */
	AADDQ,		OSIMPLE,	0x7C,	0,	-1,	-1,	0,
	ASUBQ,		OSIMPLE,	0x7D,	0,	-1,	-1,	0,
	AMULQ,		OSIMPLE,	0x7E,	0,	-1,	-1,	0,
	ADIVQ,		OSIMPLE,	0x7F,	0,	-1,	-1,	0,
	ADIVQU,		OSIMPLE,	0x80,	0,	-1,	-1,	0,
	AREMQ,		OSIMPLE,	0x81,	0,	-1,	-1,	0,
	AREMQU,		OSIMPLE,	0x82,	0,	-1,	-1,	0,
	AANDQ,		OSIMPLE,	0x83,	0,	-1,	-1,	0,
	AORQ,		OSIMPLE,	0x84,	0,	-1,	-1,	0,
	AXORQ,		OSIMPLE,	0x85,	0,	-1,	-1,	0,
	ASHLQ,		OSIMPLE,	0x86,	0,	-1,	-1,	0,
	ASHRQ,		OSIMPLE,	0x87,	0,	-1,	-1,	0,
	ASHRQU,		OSIMPLE,	0x88,	0,	-1,	-1,	0,
	AROLQ,		OSIMPLE,	0x89,	0,	-1,	-1,	0,
	ARORQ,		OSIMPLE,	0x8A,	0,	-1,	-1,	0,
	ACLZQ,		OSIMPLE,	0x79,	0,	-1,	-1,	0,
	ACTZQ,		OSIMPLE,	0x7A,	0,	-1,	-1,	0,
	APOPCNTQ,	OSIMPLE,	0x7B,	0,	-1,	-1,	0,
	ATESTQ,		OSIMPLE,	0x50,	0,	-1,	-1,	0,
	ACMPEQQ,	OSIMPLE,	0x51,	0,	-1,	-1,	0,
	ACMPNEQ,	OSIMPLE,	0x52,	0,	-1,	-1,	0,
	ACMPLTQ,	OSIMPLE,	0x53,	0,	-1,	-1,	0,
	ACMPLTQU,	OSIMPLE,	0x54,	0,	-1,	-1,	0,
	ACMPGTQ,	OSIMPLE,	0x55,	0,	-1,	-1,	0,
	ACMPGTQU,	OSIMPLE,	0x56,	0,	-1,	-1,	0,
	ACMPLEQ,	OSIMPLE,	0x57,	0,	-1,	-1,	0,
	ACMPLEQU,	OSIMPLE,	0x58,	0,	-1,	-1,	0,
	ACMPGEQ,	OSIMPLE,	0x59,	0,	-1,	-1,	0,
	ACMPGEQU,	OSIMPLE,	0x5A,	0,	-1,	-1,	0,

	/* f32 arithmetic/compare */
	AADDF,		OSIMPLE,	0x92,	0,	-1,	-1,	0,
	ASUBF,		OSIMPLE,	0x93,	0,	-1,	-1,	0,
	AMULF,		OSIMPLE,	0x94,	0,	-1,	-1,	0,
	ADIVF,		OSIMPLE,	0x95,	0,	-1,	-1,	0,
	AMINF,		OSIMPLE,	0x96,	0,	-1,	-1,	0,
	AMAXF,		OSIMPLE,	0x97,	0,	-1,	-1,	0,
	ACOPYSGNF,	OSIMPLE,	0x98,	0,	-1,	-1,	0,
	AABSF,		OSIMPLE,	0x8B,	0,	-1,	-1,	0,
	ANEGF,		OSIMPLE,	0x8C,	0,	-1,	-1,	0,
	ASQRTF,		OSIMPLE,	0x91,	0,	-1,	-1,	0,
	ACEILF,		OSIMPLE,	0x8D,	0,	-1,	-1,	0,
	AFLOORF,	OSIMPLE,	0x8E,	0,	-1,	-1,	0,
	ATRUNCF,	OSIMPLE,	0x8F,	0,	-1,	-1,	0,
	ANEARF,		OSIMPLE,	0x90,	0,	-1,	-1,	0,
	ACMPEQF,	OSIMPLE,	0x5B,	0,	-1,	-1,	0,
	ACMPNEF,	OSIMPLE,	0x5C,	0,	-1,	-1,	0,
	ACMPLTF,	OSIMPLE,	0x5D,	0,	-1,	-1,	0,
	ACMPGTF,	OSIMPLE,	0x5E,	0,	-1,	-1,	0,
	ACMPLEF,	OSIMPLE,	0x5F,	0,	-1,	-1,	0,
	ACMPGEF,	OSIMPLE,	0x60,	0,	-1,	-1,	0,

	/* f64 arithmetic/compare */
	AADDD,		OSIMPLE,	0xA0,	0,	-1,	-1,	0,
	ASUBD,		OSIMPLE,	0xA1,	0,	-1,	-1,	0,
	AMULD,		OSIMPLE,	0xA2,	0,	-1,	-1,	0,
	ADIVD,		OSIMPLE,	0xA3,	0,	-1,	-1,	0,
	AMIND,		OSIMPLE,	0xA4,	0,	-1,	-1,	0,
	AMAXD,		OSIMPLE,	0xA5,	0,	-1,	-1,	0,
	ACOPYSGND,	OSIMPLE,	0xA6,	0,	-1,	-1,	0,
	AABSD,		OSIMPLE,	0x99,	0,	-1,	-1,	0,
	ANEGD,		OSIMPLE,	0x9A,	0,	-1,	-1,	0,
	ASQRTD,		OSIMPLE,	0x9F,	0,	-1,	-1,	0,
	ACEILD,		OSIMPLE,	0x9B,	0,	-1,	-1,	0,
	AFLOORD,	OSIMPLE,	0x9C,	0,	-1,	-1,	0,
	ATRUNCD,	OSIMPLE,	0x9D,	0,	-1,	-1,	0,
	ANEARD,		OSIMPLE,	0x9E,	0,	-1,	-1,	0,
	ACMPEQD,	OSIMPLE,	0x61,	0,	-1,	-1,	0,
	ACMPNED,	OSIMPLE,	0x62,	0,	-1,	-1,	0,
	ACMPLTD,	OSIMPLE,	0x63,	0,	-1,	-1,	0,
	ACMPGTD,	OSIMPLE,	0x64,	0,	-1,	-1,	0,
	ACMPLED,	OSIMPLE,	0x65,	0,	-1,	-1,	0,
	ACMPGED,	OSIMPLE,	0x66,	0,	-1,	-1,	0,

	ALAST,		OSIMPLE,	-1,	0,	-1,	-1,	0,
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
