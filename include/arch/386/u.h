/* 386 base types for goken's own toolchain (8c/8a/8l).
 *
 * Typedefs and va_list adapted from Principia's include/arch/386/u.h
 * TODO: jmp_buf! Rune? mpdigit/FCR-FSR?
 */

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

//pad: I added that, for ed.c
//alt: remove this typedef and use ptrdiff_t which is standard
// but types without the _t suffix is more plan9ish
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

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- 386
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

/* Plan9-style va_list for 386, straight from principia's
 * include/arch/386/u.h (already validated there).
 *
 * Verified empirically against 8c -S output: unlike every arm-family
 * arch here (arm64, mips, riscv), 386 passes *no* argument in a
 * register at all -- like amd64, every argument (including the
 * callee's first) is written to the stack by the caller, so
 * syscall/arch/386/svc.s can address num+0(FP) directly. Unlike
 * amd64 though, there's no uniform-8-byte-slot rounding here either:
 * every argument packs at its exact natural sizeof() width, with no
 * padding/alignment at all (confirmed: a double placed right after an
 * int+long pair lands with zero gap, and the next int right after the
 * double also lands with zero gap) -- classic i386 weak stack
 * alignment, unlike amd64's or riscv's stricter rules. Since values
 * are little-endian, a narrower-than-4-byte value sits at the *start*
 * of its 4-byte slot (unlike big-endian mips, which needs the *end*).
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 4 ? (char*)((int*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) < 4) ? \
		((list += 4), *(mode*)(list - 4)) : \
		((list += sizeof(mode)), *(mode*)(list - sizeof(mode))))
