// Floating-point comparisons ( <, >, <=, >= ) exposed a codegen bug in
// the principia arm C compiler (compilers/5c). gopcode() declared its
// btrue flag as `bool`:
//
//     bool btrue;
//     btrue = o & BTRUE;      // BTRUE == 0x1000
//
// BTRUE is bit 12, which has no bits in the low byte, so storing
// (o & 0x1000) into a char-width bool truncates it to 0. That silently
// disabled the float-comparison NaN-safe branch substitutions
// (ABMI/ABPL/ABGT/ABHI for </>=/>/<= on floats), so 5c emitted the
// plain signed/unsigned integer conditions instead:
//
//     x < 0 :  kencc MOVW.MI/.PL   principia MOVW.LT/.GE
//     x > 0 :  kencc MOVW.GT/.LE   principia MOVW.HI/.LS  (unsigned!)
//
// Besides differing from kencc byte-for-byte, the unsigned HI/LS forms
// are wrong for NaN (e.g. NaN > 0 evaluated to 1 via HI instead of 0).
// kencc's 5c declares the same flag `int true`. Fixed in
// compilers/5c/txt.c gopcode() by declaring btrue as int.
//
// This affected 48 float-heavy files in the arm corpus sweep and was
// latent because the principia arm 5c had never been run over real
// code before.

int lt(double x) { return x <  0.0; }
int gt(double x) { return x >  0.0; }
int le(double x) { return x <= 0.0; }
int ge(double x) { return x >= 0.0; }
