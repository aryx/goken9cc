
// Those are portable defs across architectures
// The per-arch specific are in include/arch/<arch>/u.h

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef long long vlong;
typedef unsigned long long uvlong;


// the [su]8 are defined in <u.h>
//old: used to be s8int, u8int, etc. but shorted s8/u8/... like in Rust and Zig.
typedef s8 int8;
typedef u8 uint8;
typedef s16 int16;
typedef u16 uint16;
typedef s32 int32;
typedef u32 uint32;
typedef s64 int64;
typedef u64 uint64;

// intptr defined in <u.h>
//pad: I added that, for ed.c
//alt: remove this typedef and use ptrdiff_t which is standard
// but types without the _t suffix is more plan9ish
typedef intptr ptrdiff;

// fmt/fmtfd.c uses the raw C99 stdint.h name in one spot rather than
// the Plan9-style alias above; can't #include <stdint.h> (Unix header),
// so just alias it here instead of editing that ported file.
typedef uintptr uintptr_t;

// bool now in base/bool.h
