/* amd64 base types for goken's own toolchain (6c/6a/6l).
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

// claude: deliberately vlong-width, like riscv64/u.h's. 6c's `long` is
// 4 bytes (SZ_LONG in compilers/6c/gc.h), exactly like every other
// Plan9 C compiler in this tree, while pointers here are 8 (SZ_IND) --
// confirmed by probe rather than by reading gc.h: `int c =
// sizeof(uintptr);` through 6c -S emits `DATA c+0(SB)/4,$8` next to
// `sizeof(void*)`'s $8, and `sizeof(ulong)`'s $4. So `unsigned long`
// here would make uintptr 4 bytes on a 64-bit arch and silently drop
// the top 32 bits of any pointer cast through it -- port/getcallerpc.c's
// return value and port/sbrk.c's break arithmetic both go through it.
typedef unsigned long long uintptr;
typedef long long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf), adapted
// from principia's include/arch/{arm,386}/u.h (same little-endian
// layout) -- note this deliberately uses u32 for hi/lo, not ulong: u32
// states the exact width this bit layout requires, rather than relying
// on whatever `ulong` happens to be. claude: `ulong` is in fact also 4
// bytes here (6c's `long` is 4 -- see the uintptr note above), so it
// would happen to work; u32 is still the right way to write it.
union FPdbleword {
	double x;
	struct {	/* little endian */
		u32 lo;
		u32 hi;
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
