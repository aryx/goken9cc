
// Those are portable typedefs across architectures.
// The per-arch specific are in include/arch/<arch>/u.h

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

// 'long long' are C99 64 bits integers
typedef long long vlong;
typedef unsigned long long uvlong;


// The [su]8 are defined in <u.h>
// You should use the [su]xx variant in new code, like in Rust/Zig,
// but most Plan 9 code use the older and longer u?intxx variant hence
// the typedefs below.
//old: used to be s8int, u8int, etc. but s8/u8/... better, like in Rust/Zig.
typedef s8 int8;
typedef u8 uint8;
typedef s16 int16;
typedef u16 uint16;
// those 2 are really the only one used in goken/principia
typedef s32 int32;
typedef u32 uint32;
typedef s64 int64;
typedef u64 uint64;

// intptr is defined in per-arch specific <u.h>
//pad: I added ptrdiff mainly for ed.c
//alt: remove this typedef and use ptrdiff_t which is standard
// but types without the _t suffix is more plan9ish
typedef intptr ptrdiff;

// bool now in base/bool.h
