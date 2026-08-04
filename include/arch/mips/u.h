/* mips (big-endian, o32) base types for goken's own toolchain (vc/va/vl).
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

typedef unsigned long uintptr;
typedef long intptr;

// bit-level double access for port/frexp.c (frexp/ldexp/modf). unlike
// arm64/amd64/386's little-endian layout, mips here is big-endian
// (matches principia's include/arch/mips/u.h, and this project's own
// qemu-mips -- as opposed to qemu-mipsel -- target in scripts/qemu-runner);
// ulong is 4 bytes on this 32-bit arch, so no uint32-vs-ulong size
// mismatch to worry about the way arm64/amd64 have.
union FPdbleword {
	double x;
	struct {	/* big endian */
		unsigned long hi;
		unsigned long lo;
	};
};
typedef union FPdbleword FPdbleword;

/* Plan9-style va_list for mips, straight from principia's
 * include/arch/mips/u.h (already validated there).
 *
 * Verified empirically against vc -S on throwaway probes: this arch's
 * ABI passes only the *first* named argument in a register (R1) --
 * like arm64, a compiler-generated function only spills it to its
 * home slot (arg+0(FP)) if its address is actually taken, so
 * hand-written assembly (see syscall/arch/mips/svc.s) must read it
 * from R1 directly, never via FP. Unlike arm64/amd64 though, every
 * *other* argument (named or variadic) is packed at its natural
 * sizeof() width with no padding/rounding -- an int leaves only 4
 * bytes before the next slot, not 8 (confirmed: a double placed right
 * after two int args lands exactly 8 bytes along, without any extra
 * alignment gap) -- hence va_arg here just advances by sizeof(mode),
 * only rounding up the sub-int (<4 byte) cases to a 4-byte slot.
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 4 ? (char*)((int*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) < 4) ? ((list += 4), (mode*)list)[-1] : \
	 ((list += sizeof(mode)), (mode*)list)[-1])
