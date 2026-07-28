/* arm (32-bit) base types for goken's own toolchain (5c/5a/5l).
 *
 * Freestanding: no Unix/host headers (lib_core/ code may only depend on
 * headers under include/, never on the host's libc -- that's lib9/'s job).
 *
 * Typedefs and va_list adapted from ~/principia/include/arch/arm/u.h
 * (a known-working reference for this exact arch) -- trimmed to what
 * lib_core/libc actually uses so far (no Rune/jmp_buf/mpdigit/FCR-FSR
 * flags yet, matching the same policy as arm64's/mips's/riscv's u.h).
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

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- arm
// is little-endian; ulong is 4 bytes here, matching uint32, no
// arm64-style size mismatch to worry about.
union FPdbleword {
	double x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};
typedef union FPdbleword FPdbleword;

/* Plan9-style va_list for arm, straight from principia's
 * include/arch/arm/u.h (already validated there).
 *
 * Verified empirically against 5c -S output: like arm64/mips/riscv,
 * only the *first* named parameter arrives in a register (R0) -- a
 * compiler-generated function only spills it to its home slot
 * (arg+0(FP)) if its address is actually taken, so hand-written
 * assembly (see syscall/arch/arm/svc.s) must read it from R0
 * directly. Unlike riscv though (and matching mips/386), every other
 * argument packs at its exact natural sizeof() width with no
 * 8-byte-alignment rounding at all (confirmed: a double placed right
 * after an int+long pair lands with zero gap). Since values are
 * little-endian, a narrower-than-4-byte value sits at the *start* of
 * its 4-byte slot (unlike big-endian mips, which needs the *end*).
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 4 ? (char*)((int*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) < 4) ? \
		((list += 4), *(mode*)(list - 4)) : \
		((list += sizeof(mode)), *(mode*)(list - sizeof(mode))))
