/* arm64 base types for goken's own toolchain (7c/7a/7l).
 */

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

typedef float float32;
typedef double float64;

//TODO? not unsigned long long like in riscv64/u.h? see the comment there
typedef unsigned long uintptr;
typedef long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf), adapted
// from principia's include/arch/{arm,386}/u.h (same little-endian
// layout) -- note this deliberately uses uint32 for hi/lo, not ulong:
// ulong is 8 bytes on this arch (unlike 32-bit arm/386, where it's the
// same size as uint32), which would silently double the union's size.
union FPdbleword {
	double x;
	struct {	/* little endian */
		u32 lo;
		u32 hi;
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
