
// those are portable defs across architectures

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef long long vlong;
typedef unsigned long long uvlong;


// the [su]8 defined in u.h
//old: used to be s8int, u8int, etc. but shorted s8/u8/... like in Rust and Zig.
typedef s8 int8;
typedef u8 uint8;
typedef s16 int16;
typedef u16 uint16;
typedef s32 int32;
typedef u32 uint32;
typedef s64 int64;
typedef u64 uint64;

// intptr defined in u.h
//pad: I added that, for ed.c
//alt: remove this typedef and use ptrdiff_t which is standard
// but types without the _t suffix is more plan9ish
typedef intptr ptrdiff;

// fmt/fmtfd.c uses the raw C99 stdint.h name in one spot rather than
// the Plan9-style alias above; can't #include <stdint.h> (Unix header),
// so just alias it here instead of editing that ported file.
typedef uintptr uintptr_t;

#ifndef __bool_true_false_are_defined
//Note that it is dangerous to use u8 for a bool and to switch some
// 'int flag;' to 'bool flag;' in some legacy code because this code
// might actually use this flag in an integer context and using a bool
// might truncate some operations now.
// So take care when transitioning code to bool!
typedef u8 bool;
enum {
	false = 0,
	true = 1,
};
#define __bool_true_false_are_defined 1
#endif
