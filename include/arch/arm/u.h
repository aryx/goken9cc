/* arm (32-bit) base types for goken's own toolchain (5c/5a/5l).
 *
 * Freestanding: no Unix/host headers (lib_core/ code may only depend on
 * headers under include/, never on the host's libc -- that's lib9/'s job).
 *
 * Typedefs and va_list adapted from ~/principia/include/arch/arm/u.h
 * TODO: Rune/jmp_buf/mpdigit/FCR-FSR
 */

//old: used to be s8int, u8int, etc. but shorter s8/u8/... like in Rust and Zig.
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

typedef unsigned long uintptr;
typedef long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf) -- arm
// is little-endian; ulong is 4 bytes here, matching uint32, no
// arm64-style size mismatch to worry about.
union FPdbleword {
	double x;
	struct {	/* little endian */
		unsigned long lo;
		unsigned long hi;
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
