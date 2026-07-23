/*
 * wasm (WebAssembly), arch letter 'e'.
 *
 * claude: modeled on include/5.out.h, include/6.out.h, include/8.out.h,
 * and on linkers/vl/v.out.h and linkers/il/i.out.h (the two most
 * recently-added arches). Follows v.out.h/i.out.h specifically for the
 * D_* enum: one enum namespace, split across Gen's `type` and `name`
 * fields (type = operand encoding, name = memory region for a D_OREG),
 * rather than either arm's two separate enum types (Operand_kind/
 * Sym_kind) or x86/amd64's single additive-D_INDIR field.
 *
 * Scope: the WASM MVP (2017) instruction set only -- i32/i64/f32/f64,
 * linear memory, structured control flow. No SIMD (v128), no
 * threads/atomics, no exception-handling or tail-call proposals: none
 * of those are needed to compile the C subset this toolchain targets,
 * and they can be added later the same way other arches grew opcodes.
 *
 * claude: wasm has no fixed register file, which changes the shape of
 * this header more than any single-arch difference elsewhere in the
 * project:
 *   - No REGRET/REGSP/REGTMP-as-hardware-register. Call arguments and
 *     results simply live on wasm's implicit operand stack, in the
 *     order given by the function's type -- there is no register to
 *     name for "the return value".
 *   - What every other arch calls a "register" (D_REG) splits into two
 *     unrelated wasm concepts here: D_LOCAL (a typed, per-function
 *     local, addressed by index -- there is no cross-function/global
 *     register file, and no spilling: a fresh local can be allocated
 *     per live temporary) and D_GLOBAL (a typed, module-level global;
 *     currently only used for the shadow stack pointer, see SPGLOBAL
 *     below).
 *   - A wasm local's *address* cannot be taken (unlike a stack slot on
 *     a real machine). So D_AUTO/D_PARAM keep their usual meaning only
 *     for values that never have their address taken (compiled to a
 *     true D_LOCAL); anything address-taken must instead live in a
 *     conventional linear-memory "shadow stack", addressed relative to
 *     SPGLOBAL exactly like D_AUTO/D_PARAM work on every other arch.
 *     Telling those two cases apart is ec's job, not this header's.
 *   - D_BRANCH's value is a structured-control label depth (how many
 *     enclosing block/loop/if constructs to exit), not a PC/address --
 *     wasm has no unstructured jump. The assembler resolves depths from
 *     ABLOCK/ALOOP/AIF/AENDCTL nesting, the same way other arches'
 *     assemblers resolve D_BRANCH into a PC-relative displacement.
 *   - No NREG/REGMIN/REGMAX/REGEXT-style register-allocation range:
 *     there is nothing to allocate from. No NOSPLIT either: wasm's
 *     call stack is host-managed and can't be segmented/grown the way
 *     NOSPLIT guards against on a real machine.
 *
 * claude: no virtual AMOVx here, unlike an earlier draft of this file.
 * Every other arch's AMOVx works as one 2-operand mnemonic because
 * there's always a *register* to be the other side of a load, a
 * store, or a constant load -- the same instruction, just aimed at
 * different places. wasm has no register to play that role, so
 * forcing everything through one MOV-shaped mnemonic just made the
 * arity silently change meaning (sometimes one operand meaning "push
 * and leave it on the stack", sometimes two meaning "move"), which is
 * exactly the kind of surprise a programmer coming from 5a/8a/ia
 * would *not* expect from something called MOVW. Instead, each opcode
 * below matches one real wasm instruction, with exactly the operand
 * *that instruction's own encoding* carries -- see each section's
 * comment. What still carries over deliberately: SB/SP/FP (still
 * meaningful pseudo-registers), TEXT/GLOBL/DATA/WORD (identical
 * concept), and CALL/RET/BR (still "control transfer with an
 * operand").
 */

/*
 * claude: NSYM and Ieee are NOT redefined here (unlike v.out.h/i.out.h,
 * which are self-contained headers with no other shared include).
 * e.out.h is meant to be reached the way 5.out.h/8.out.h are: through
 * assemblers/Xa's shared aa.h (assemblers/as/aa.h), which already
 * pulls in include/common.out.h for both. Including this header any
 * other way must arrange for common.out.h itself first.
 */
#define	NSNAME	8

#define	NOPROF	(1<<0)
#define	DUPOK	(1<<1)

/*
 * claude: cck/pgen.c does `if (REGARG >= 0)` to decide whether to pass
 * the first argument in a register instead of on the stack (see the
 * REGARG comment in include/6.out.h for the history of that mismatch
 * in this project). wasm has no register to pass it in, so -1 here
 * isn't just consistency with 8c/6c/5c/vc/ic in this project -- it's
 * the only correct value.
 */
#define	REGARG	-1

/*
 * The shadow stack pointer: a mutable wasm global (not a register --
 * wasm has none), holding the linear-memory address of the current
 * frame's shadow stack, the same role REGSP plays on every other
 * arch. Index 0 assumes it is the first global the module declares;
 * el is what actually assigns and enforces that.
 */
#define	SPGLOBAL	0

/*
 * type/name, one enum namespace split across Gen's two fields, same
 * convention as v.out.h/i.out.h: Gen.type holds one of the "type"
 * values below and says how the instruction encodes the operand;
 * Gen.name holds one of the "name" values and, only when
 * type==D_OREG, says which linear-memory region a symbol is relative
 * to. Unlike x86/amd64's D_INDIR, name is not additive onto type --
 * there is exactly one symbol-reference type (D_OREG), because
 * D_LOCAL/D_GLOBAL (wasm locals/globals) never take a name: they are
 * never memory references, so the distinction doesn't apply to them.
 *
 * claude: D_OREG no longer means "the address a load/store reads or
 * writes" the way it did in an earlier draft (see the AMOVx comment
 * above) -- ALOADx/ASTOREx below take a plain offset, not an address,
 * because that's what their real wasm encoding carries. D_OREG now
 * means "a reference to a symbol's address", used only where that's
 * still genuinely needed: ea's grammar folds a `name(SB)` operand
 * into a D_OREG while parsing ACALL's target or ACONSTx's
 * address-of-symbol form (see those opcodes' comments), and ACALL's
 * target stays D_OREG all the way into the object file, for el to
 * resolve into a function index.
 */
enum
{
	D_GOK	= 0,
	D_NONE,

	/* name: which linear-memory region a D_OREG symbol lives in */
	D_EXTERN,	/* data/bss (from the module's data base) */
	D_STATIC,	/* file-static data (private symbol) */
	D_AUTO,		/* address-taken local: shadow-stack slot (from SPGLOBAL) */
	D_PARAM,	/* address-taken param: shadow-stack slot (from SPGLOBAL) */

	/* type: how the instruction encodes the operand */
	D_BRANCH,	/* structured-control label depth, not a PC (see above) */
	D_OREG,		/* a symbol reference (see the comment above) */
	D_CONST,
	D_VCONST,	/* i64.const that doesn't fit in Gen.offset; see Gen.vval */
	D_FCONST,
	D_SCONST,
	D_LOCAL,	/* true wasm local, by index (replaces D_REG) */
	D_GLOBAL,	/* module-level wasm global, by index (e.g. SPGLOBAL) */
	D_FILE,
	D_FILE1,	/* linker only */
};

enum	as
{
	AXXX,

	/*
	 * Structured control flow. AENDCTL closes a block/loop/if/
	 * function (wasm's single `end` opcode) -- named to avoid
	 * colliding with the AEND pseudo-op below, which every other
	 * arch already uses for "end of this function's instruction
	 * stream" in the Plan9 object-file sense.
	 */
	ANOP,
	AUNREACHABLE,
	ABLOCK,
	ALOOP,
	AIF,
	AELSE,
	AENDCTL,
	ABR,
	ABRIF,
	ABRTABLE,
	ARET,
	ACALL,
	ACALLIND,
	ADROP,
	ASELECT,

	/*
	 * Locals and globals: one operand each, the index -- genuinely
	 * part of the real instruction's encoding (local.get/local.set/
	 * local.tee/global.get/global.set all carry one), so this isn't
	 * the same kind of mismatch AMOVx was. A wasm local is this
	 * arch's closest equivalent of a register (unbounded, but a
	 * "slot" in exactly the sense SP/SB/FP are already pseudo, not
	 * hardware); ea's grammar spells it `LOCAL(n)`/`GLOBAL(n)`.
	 */
	ALOCALGET,
	ALOCALSET,
	ALOCALTEE,
	AGLOBALGET,
	AGLOBALSET,

	/* linear memory management */
	AMEMSIZE,
	AMEMGROW,

	/*
	 * Push an immediate: one operand, the value (or a symbol's
	 * address, folded in as a D_CONST the same way riscv/mips's own
	 * `imm: '$' addr` does for a constant that's really a symbol
	 * reference) -- again genuinely part of iNN.const/fNN.const's
	 * own encoding.
	 */
	ACONSTW,
	ACONSTQ,
	ACONSTF,
	ACONSTD,

	/*
	 * Load and store: one operand, the memarg *offset* -- what the
	 * real i32.load/i32.store encoding actually carries. The base
	 * address is never an operand of these opcodes; it comes off
	 * the stack, pushed by whatever precedes the load/store (an
	 * ACONSTx for a symbol's address, or an AGLOBALGET of SPGLOBAL
	 * for a stack-relative one) -- the same requirement WAT's own
	 * nested `(i32.store (i32.const addr) (i32.const val))` encodes
	 * by evaluating its first child before its second. ea's grammar
	 * still accepts the familiar `name(SB)`/`off(SP)`/`off(FP)`
	 * syntax and expands it into that push-then-load/store sequence
	 * itself (see a.y), so the source still reads like every other
	 * arch's addressing even though the object file underneath is
	 * honest about there being two real instructions.
	 */
	ALOADB,		/* i32.load8_s */
	ALOADBU,	/* i32.load8_u */
	ALOADH,		/* i32.load16_s */
	ALOADHU,	/* i32.load16_u */
	ALOADW,		/* i32.load */
	ALOADQ,		/* i64.load */
	ALOADBQ,	/* i64.load8_s */
	ALOADBUQ,	/* i64.load8_u */
	ALOADHQ,	/* i64.load16_s */
	ALOADHUQ,	/* i64.load16_u */
	ALOADWQ,	/* i64.load32_s */
	ALOADWUQ,	/* i64.load32_u */
	ALOADF,		/* f32.load */
	ALOADD,		/* f64.load */

	ASTOREB,	/* i32.store8 (also used to store the low byte of an i64) */
	ASTOREH,	/* i32.store16 (also i64) */
	ASTOREW,	/* i32.store */
	ASTOREWQ,	/* i64.store32: low 32 bits of an i64 value */
	ASTOREQ,	/* i64.store */
	ASTOREF,	/* f32.store */
	ASTORED,	/* f64.store */

	/* i32 arithmetic/logic/compare */
	AADDW,
	ASUBW,
	AMULW,
	ADIVW,
	ADIVWU,
	AREMW,
	AREMWU,
	AANDW,
	AORW,
	AXORW,
	ASHLW,
	ASHRW,
	ASHRWU,
	AROLW,
	ARORW,
	ACLZW,
	ACTZW,
	APOPCNTW,
	ATESTW,		/* i32.eqz */
	ACMPEQW,
	ACMPNEW,
	ACMPLTW,
	ACMPLTWU,
	ACMPGTW,
	ACMPGTWU,
	ACMPLEW,
	ACMPLEWU,
	ACMPGEW,
	ACMPGEWU,

	/* i64 arithmetic/logic/compare */
	AADDQ,
	ASUBQ,
	AMULQ,
	ADIVQ,
	ADIVQU,
	AREMQ,
	AREMQU,
	AANDQ,
	AORQ,
	AXORQ,
	ASHLQ,
	ASHRQ,
	ASHRQU,
	AROLQ,
	ARORQ,
	ACLZQ,
	ACTZQ,
	APOPCNTQ,
	ATESTQ,		/* i64.eqz */
	ACMPEQQ,
	ACMPNEQ,
	ACMPLTQ,
	ACMPLTQU,
	ACMPGTQ,
	ACMPGTQU,
	ACMPLEQ,
	ACMPLEQU,
	ACMPGEQ,
	ACMPGEQU,

	/* f32 arithmetic/compare */
	AADDF,
	ASUBF,
	AMULF,
	ADIVF,
	AMINF,
	AMAXF,
	ACOPYSGNF,
	AABSF,
	ANEGF,
	ASQRTF,
	ACEILF,
	AFLOORF,
	ATRUNCF,
	ANEARF,
	ACMPEQF,
	ACMPNEF,
	ACMPLTF,
	ACMPGTF,
	ACMPLEF,
	ACMPGEF,

	/* f64 arithmetic/compare */
	AADDD,
	ASUBD,
	AMULD,
	ADIVD,
	AMIND,
	AMAXD,
	ACOPYSGND,
	AABSD,
	ANEGD,
	ASQRTD,
	ACEILD,
	AFLOORD,
	ATRUNCD,
	ANEARD,
	ACMPEQD,
	ACMPNED,
	ACMPLTD,
	ACMPGTD,
	ACMPLED,
	ACMPGED,

	/*
	 * claude: register-only conversions, zero operands -- unlike
	 * every opcode above with an operand, i32.wrap_i64,
	 * i64.extend_i32_s, f32.convert_i32_s and the rest all carry
	 * *no* immediate in their real wasm encoding; they only ever
	 * transform whatever's already on the stack. Named after the
	 * wasm spec's own words for these (wrap/extend/convert/trunc/
	 * promote/demote), not "MOV", since there's no addressing or
	 * moving happening at all.
	 */
	AWRAPQ,		/* i32.wrap_i64 */
	AEXTW,		/* i64.extend_i32_s */
	AEXTWU,		/* i64.extend_i32_u */

	ACONVWF,	/* f32.convert_i32_s */
	ACONVWUF,	/* f32.convert_i32_u */
	ACONVWD,	/* f64.convert_i32_s */
	ACONVWUD,	/* f64.convert_i32_u */
	ACONVQF,	/* f32.convert_i64_s */
	ACONVQUF,	/* f32.convert_i64_u */
	ACONVQD,	/* f64.convert_i64_s */
	ACONVQUD,	/* f64.convert_i64_u */

	ATRUNCFW,	/* i32.trunc_f32_s */
	ATRUNCFWU,	/* i32.trunc_f32_u */
	ATRUNCFQ,	/* i64.trunc_f32_s */
	ATRUNCFQU,	/* i64.trunc_f32_u */
	ATRUNCDW,	/* i32.trunc_f64_s */
	ATRUNCDWU,	/* i32.trunc_f64_u */
	ATRUNCDQ,	/* i64.trunc_f64_s */
	ATRUNCDQU,	/* i64.trunc_f64_u */

	APROMOTE,	/* f64.promote_f32 */
	ADEMOTE,	/* f32.demote_f64 */

	/* bit-level reinterpretation (union type punning, fabs/copysign helpers) */
	AREINTWF,	/* i32 bits -> f32 */
	AREINTFW,	/* f32 bits -> i32 */
	AREINTQD,	/* i64 bits -> f64 */
	AREINTDQ,	/* f64 bits -> i64 */

	/* C compiler pseudo-ops (same set/order as every other arch) */
	ATEXT,
	AGLOBL,
	ADATA,
	AWORD,
	ANAME,
	AHISTORY,
	AEND,
	ADYNT,
	AINIT,
	ASIGNAME,
	AGOK,

	ALAST,
};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"
