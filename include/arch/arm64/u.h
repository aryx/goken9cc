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

/* claude: classic Plan9 fixed-width aliases (u8int/u16int/u32int/u64int),
 * distinct from the u8/u16/u32/u64 names this file already defines --
 * some ported Plan9 sources (e.g. utilities/archive/tar/tar.c) use the
 * *int-suffixed spelling directly. Found self-hosting tar.c with
 * goken's own compiler+libc.
 */
typedef u8 u8int;
typedef u16 u16int;
typedef u32 u32int;
typedef u64 u64int;

typedef float float32;
typedef double float64;

// claude: deliberately vlong-width, like riscv64/u.h's and amd64/u.h's
// (see the latter for the full reasoning). 7c's `long` is 4 bytes
// (SZ_LONG in compilers/7c/gc.h) while pointers are 8 (SZ_IND), so
// `unsigned long` here would make uintptr 4 bytes on a 64-bit arch and
// truncate every pointer cast through it.
typedef unsigned long long uintptr;
typedef long long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf), adapted
// from principia's include/arch/{arm,386}/u.h (same little-endian
// layout) -- note this deliberately uses u32 for hi/lo, not ulong: u32
// states the exact width this bit layout requires, rather than relying
// on whatever `ulong` happens to be. claude: `ulong` is in fact also 4
// bytes here (7c's `long` is 4 -- see the uintptr note above). See
// amd64/u.h's identical note.
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
