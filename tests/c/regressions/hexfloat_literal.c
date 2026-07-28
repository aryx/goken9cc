// hexfloat_literal: found while porting benchs/compcert/bisect.c to
// build under goken's own toolchain. bisect.c defines
// `#define DBL_EPSILON 0x1p-52` (a C99 hexadecimal floating
// constant -- "0x1" times 2 to the power of -52, i.e. 2^-52) and uses
// it later in an expression. This compiler's lexer doesn't understand
// the C99 hex-float suffix (the "p-52" exponent part) at all: it
// fails with a syntax error at the *use* site, not the #define line
// itself (the preprocessor just stores the raw "0x1p-52" text without
// looking at it; the failure is in the compiler proper's later
// attempt to lex that substituted text as a numeric literal). Tried
// (and confirmed does NOT help): pre-defining DBL_EPSILON to the
// equivalent decimal literal in a header included before bisect.c's
// own #define -- the redefinition itself errors too (same class as
// this project's own PI-macro collision, see
// docs/claude_notes/notes_libc_selfhost.txt), and the hex-float text
// still fails to parse once substituted in. Not a missing declaration
// or a macro-ordering problem -- a genuine lexer/grammar gap for this
// literal syntax, not investigated further than confirming the
// symptom and that it's specifically the "p" exponent suffix (a plain
// "0x1" alone, or a decimal float like "2.2204460492503131e-16" --
// the exact value bisect.c's own main() already writes out longhand a
// few lines below its DBL_EPSILON #define -- both parse fine).
//
// Confirmed real: "Xc -c hexfloat_literal.c" (any of 5c/6c/7c/8c/vc/ic
// -- this is checked on 7c below, but nothing in the failing code path
// is arm64-specific; see the comment above for why) exits nonzero with
// a syntax error, not a clean compile, before any fix. This file only
// isolates and pins down the symptom -- no runtime check, since the
// bug is a compile-time failure (same shape as this directory's own
// mulzero.c, which also checks "did it compile" rather than a runtime
// result).
//
// Not wired into this directory's `test:V:` (see mkfile's own
// comment): still an open bug, not a fixed one to guard. Verify by
// hand once a fix lands: "7c -c hexfloat_literal.c" (want exit 0, no
// diagnostic); repeat for 5c/6c/8c/vc/ic too if the fix landed
// somewhere shared by all of them rather than 7c specifically.

double dbl_epsilon_as_hexfloat = 0x1p-52;
