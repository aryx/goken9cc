/* amd64 base types for goken's own toolchain (6c/6a/6l).
 */

typedef signed char s8int;
typedef unsigned char u8int;
typedef signed short s16int;
typedef unsigned short u16int;
typedef signed int s32int;
typedef unsigned int u32int;
typedef signed long long s64int;
typedef unsigned long long u64int;

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
