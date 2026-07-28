/* arm64 base types for goken's own toolchain (7c/7a/7l).
 *
 * Freestanding: no Unix/host headers (lib_core/ code may only depend on
 * headers under include/, never on the host's libc -- that's lib9/'s job).
 */

#define nil ((void*)0)

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef long long vlong;
typedef unsigned long long uvlong;

typedef signed char s8int;
typedef unsigned char u8int;
typedef signed short s16int;
typedef unsigned short u16int;
typedef signed int s32int;
typedef unsigned int u32int;
typedef signed long long s64int;
typedef unsigned long long u64int;

typedef s8int int8;
typedef u8int uint8;
typedef s16int int16;
typedef u16int uint16;
typedef s32int int32;
typedef u32int uint32;
typedef s64int int64;
typedef u64int uint64;

typedef float float32;
typedef double float64;

typedef unsigned long uintptr;
typedef long intptr;
typedef intptr ptrdiff;

// fmt/fmtfd.c uses the raw C99 stdint.h name in one spot rather than
// the Plan9-style alias above; can't #include <stdint.h> (Unix header),
// so just alias it here instead of editing that ported file.
typedef uintptr uintptr_t;

#ifndef __bool_true_false_are_defined
typedef uint8 bool;
enum {
	false = 0,
	true = 1,
};
#define __bool_true_false_are_defined 1
#endif

typedef uint8 byte;

// bit-level double access for port/frexp.c (frexp/ldexp/modf), adapted
// from principia's include/arch/{arm,386}/u.h (same little-endian
// layout) -- note this deliberately uses uint32 for hi/lo, not ulong:
// ulong is 8 bytes on this arch (unlike 32-bit arm/386, where it's the
// same size as uint32), which would silently double the union's size.
union FPdbleword {
	double x;
	struct {	/* little endian */
		uint32 lo;
		uint32 hi;
	};
};
typedef union FPdbleword FPdbleword;

/* Plan9-style va_list for arm64, adapted from principia's
 * include/arch/{arm,mips}/u.h (4-byte-slot 32-bit convention) to this
 * arch's slot width.
 *
 * Verified empirically against 7c -S on throwaway probes: every named
 * *and* variadic argument occupies a full 8-byte-aligned stack slot on
 * this arch, regardless of its actual type width (e.g. an int32 parameter
 * still gets the same 8-byte stride as a following int64/pointer/double
 * one) -- so va_start/va_arg must round any type smaller than 8 bytes up
 * to 8 before doing the pointer arithmetic; types already >= 8 bytes
 * (long, double, pointers) match the compiler's own natural pointer
 * arithmetic and need no special-casing.
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 8 ? (char*)((vlong*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) == 1) ? ((list += 8), (mode*)list)[-8] : \
	 (sizeof(mode) == 2) ? ((list += 8), (mode*)list)[-4] : \
	 (sizeof(mode) == 4) ? ((list += 8), (mode*)list)[-2] : \
	 ((list += sizeof(mode)), (mode*)list)[-1])
