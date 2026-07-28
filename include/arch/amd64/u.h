/* amd64 base types for goken's own toolchain (6c/6a/6l).
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

/* Plan9-style va_list for amd64.
 *
 * Verified empirically against 6c -S on throwaway probes (both reading
 * a fixed-arg function's own FP offsets, and the caller-side stack
 * layout at a variadic call site): like arm64, every named argument
 * occupies a full 8-byte-aligned stack slot regardless of its actual
 * type width (a preceding int32 still leaves the next arg 8 bytes
 * further along, not 4) -- unlike arm64 though, this isn't a case of
 * rounding a *register-passed* first argument's home slot up to 8;
 * on amd64 there simply is no register-passed argument at all, the
 * caller always writes every argument (including the callee's very
 * first one) to the stack before the call, so a hand-written function
 * (see syscall/arch/amd64/svc.s) can address num+0(FP) directly, with
 * no compiler-prologue spill trick needed the way arm64's/mips's is.
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
