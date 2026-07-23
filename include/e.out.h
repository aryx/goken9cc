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
 * type==D_OREG, says which linear-memory region the offset is
 * relative to. Unlike x86/amd64's D_INDIR, name is not additive onto
 * type -- there is exactly one memory-operand type (D_OREG), because
 * D_LOCAL/D_GLOBAL (wasm locals/globals) never take a name: they are
 * never memory references, so the distinction doesn't apply to them.
 */
enum
{
	D_GOK	= 0,
	D_NONE,

	/* name: which linear-memory region a D_OREG offset is relative to */
	D_EXTERN,	/* data/bss (from the module's data base) */
	D_STATIC,	/* file-static data (private symbol) */
	D_AUTO,		/* address-taken local: shadow-stack slot (from SPGLOBAL) */
	D_PARAM,	/* address-taken param: shadow-stack slot (from SPGLOBAL) */

	/* type: how the instruction encodes the operand */
	D_BRANCH,	/* structured-control label depth, not a PC (see above) */
	D_OREG,		/* linear-memory operand: offset, relative to Gen.name */
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
	 * claude: local.tee has no natural "MOV" pairing (it reads
	 * *and* writes the same local without popping, unlike a plain
	 * move), so it keeps its own dedicated one-operand mnemonic --
	 * the same way BL/SWI/CALL stand apart from arm's MOV family.
	 * Plain local/global read and write, though, fold into the
	 * unified AMOVx family below, the same way REGRET/REGARG never
	 * needed their own opcodes on any other arch: a wasm local is
	 * this arch's pseudo-register (unbounded, but a "slot" in
	 * exactly the sense SP/SB/FP are already pseudo, not hardware),
	 * so it takes the same MOV mnemonics real registers would.
	 */
	ALOCALTEE,

	/* linear memory management */
	AMEMSIZE,
	AMEMGROW,

	/*
	 * claude: virtual, Plan9-style, like arm/x86/riscv/mips's own
	 * AMOVx (see 5.out.h's "AMOVW, // VIRTUAL, transformed in load
	 * and store instructions"): one opcode per width/sign, and ea's
	 * grammar accepts it with an immediate, a memory address
	 * (D_OREG), a local (D_LOCAL), or a global (D_GLOBAL) on either
	 * side -- el picks the concrete wasm instruction sequence
	 * (const/load/local.get/global.get, then local.set/global.set/
	 * store) from the operand kinds, exactly as arm/x86/riscv/mips
	 * pick LDR vs STR vs a register move from theirs.
	 *
	 * One genuine wasm-specific extension of the convention: a
	 * *one*-operand form ("MOVW src") is also allowed, meaning
	 * "push src, leave it on the stack" -- needed because wasm's
	 * calling convention passes arguments via a strictly-ordered
	 * push sequence right before ACALL, not through named/
	 * addressable storage the way a real stack frame would (there
	 * is no REGARG-style register or stack slot for el to read
	 * them back from afterwards).
	 */
	AMOVB,		/* byte, sign-extend to i32 */
	AMOVBU,		/* byte, zero-extend to i32 */
	AMOVH,		/* halfword, sign-extend to i32 */
	AMOVHU,		/* halfword, zero-extend to i32 */
	AMOVW,		/* i32, no extension */
	AMOVQ,		/* i64, no extension */
	AMOVF,		/* f32 */
	AMOVD,		/* f64 */
	/* widening into i64 */
	AMOVBQ,		/* byte, sign-extend to i64 */
	AMOVBUQ,	/* byte, zero-extend to i64 */
	AMOVHQ,		/* halfword, sign-extend to i64 */
	AMOVHUQ,	/* halfword, zero-extend to i64 */
	AMOVWQ,		/* i32, sign-extend to i64 (i64.extend_i32_s) */
	AMOVWUQ,	/* i32, zero-extend to i64 (i64.extend_i32_u) */
	/* narrowing i64 to i32 */
	AMOVQW,		/* i64 to i32, wrap (i32.wrap_i64) */

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

	/* int <-> float conversions */
	AMOVWF,		/* i32 -> f32, signed */
	AMOVWUF,	/* i32 -> f32, unsigned */
	AMOVWD,		/* i32 -> f64, signed */
	AMOVWUD,	/* i32 -> f64, unsigned */
	AMOVQF,		/* i64 -> f32, signed */
	AMOVQUF,	/* i64 -> f32, unsigned */
	AMOVQD,		/* i64 -> f64, signed */
	AMOVQUD,	/* i64 -> f64, unsigned */
	AMOVFW,		/* f32 -> i32, signed trunc */
	AMOVFWU,	/* f32 -> i32, unsigned trunc */
	AMOVFQ,		/* f32 -> i64, signed trunc */
	AMOVFQU,	/* f32 -> i64, unsigned trunc */
	AMOVDW,		/* f64 -> i32, signed trunc */
	AMOVDWU,	/* f64 -> i32, unsigned trunc */
	AMOVDQ,		/* f64 -> i64, signed trunc */
	AMOVDQU,	/* f64 -> i64, unsigned trunc */
	AMOVFD,		/* f32 -> f64, promote */
	AMOVDF,		/* f64 -> f32, demote */

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
